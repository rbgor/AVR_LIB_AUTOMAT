/**
 * "Сердечко". Вариант КА на базе макроса everyOVF().
 * размер скетча при компиляции 850 байт. SRAM = 33 байт.
 *
 * Аппаратное, плавное управление яркостью (ШИМ - широтно-импульсная модуляция, PWM)
 * Изменяем яркость на светодиоде за счет изменения ширины (длительности) импульса (1)
 * и его паузы (0) при одной и той же тактовой частоте следования импульсов (тут стандартно 16МГц/64/256=976Гц.)
 *
 * "Сердечный ритм" задается последовательностью предельных значений яркости (массив starts)
 * и набором шагов (скорости) изменения яркости (ширины импульсов). На каждой стадии цикла
 * из массива starts[] выбираются 2 соседних элемента для определения "начала" и "конца" стадии.
 *
 * Дополнительно подключаем КА "встроенный светодиод", который будет моргать по секундно.
 * 
 * Примечание:
 * поскольку чиселки небольшие, то крайние значения надо выбирать так, чтобы добавляя или убавляя
 * шаг не выйти через переполнение (<256). В противном случае, получится не то что вы ожидали увидеть.
 *
 * @author Arhat109 arhat109@mail.ru, +7-(951)-388-2793
 * 
 * Лицензия:
 * 1. Полностью свободное и бесплатное программное обеспечение. В том числе и от претензий.
 * 2. Вы вправе использовать его на свои нужды произвольным образом и на свой риск.
 * 3. Вы не вправе удалять из него строку с тегом @author или изменять её.
 * 4. Изменяя этот файл, Вы вправе дописать свои авторские данные и/или пояснения.
 * 
 * Если Вам это оказалось полезным, то Вы можете по-достоинству оценить мой труд
 * "на свое усмотрение" (напр. кинуть денег на телефон "сколько не жалко")
 */

#include "arhat.h"

#define pinHeart        pin4    // нога со светодиодом для плавного управления яркостью
#define WAIT_STEP       29      // тиков, 30 миллисекунд на 1 шаг изменения яркости
#define WAIT_CYCLE      156     // .., 160 миллисекунд паузы между повторением
#define WAIT_ON         98      // .., 100 мсек интервал включение контрольного светодиода (pinLed = 13)
#define WAIT_OFF        878     // .., 900 мсек интервал выключения контрольного светодиода

// Текущие данные всех КА объявляем статически. Они не должны меняться между вызовами функции КА:
// начальные значение устанавливаем в setup() чтобы компилятор не создавал доп. код по переносу данных
// из программной памяти flash в оперативную sram

// ======== КА blink: моргалка контрольным светодиодом ========= //
uint32_t blinkWait;             // текущий интервал ожидания. WAIT_ON или WAIT_OFF могут отличаться!
uint8_t  blinkState;            // текущее состояние светодиода. НЕЛЬЗЯ прочитать через digitalRead() !!!

// ======== КА heart: сердечко ========= //

// пределы яркости: каждая пара соседних чисел: "от".."до"
// сначала плавно растим яркость от 10 до 240, затем гасим от 240 до 20,
// снова увеличиваем от 20 до 230 и потом уменьшаем от 230 до 10
// ... и далее "по кругу". Конец всегда должен быть равен началу.
uint8_t starts[] = {10,230,20,220,10};

// скорость изменения яркости: шагов меньше на один чем пределов!
#define MAX_STEPS   4
uint8_t steps[] = {20,15,15,10};

// Счетчики стадии сердечка, текущая яркость и текущее время ожидания
uint32_t heartWait;
uint8_t heartState;
uint8_t heartLight;

// Пример действий КА в виде "функции переходов" между его состояниями
void doHeart()
{
  uint8_t curMax  = starts[heartState];
  uint8_t nextMax = starts[heartState+1];

  if( nextMax > curMax ){                               // яркость - увеличиваем?
    heartLight += steps[heartState];
    if( heartLight >= nextMax ){                        // . дошли до максиума яркости стадии?
      ++heartState;                                     // .. переходим на след. стадию.
    }
  } else {
    heartLight -= steps[heartState];
    if( heartLight <= nextMax ){                        // . уменьшаем текущую яркость. Миниум?
      ++heartState;                                     // .. переходим на след. стадию.
    }
  }
  heartWait = WAIT_STEP;                                // интервал ожидания следующего шага КА
  if( heartState >= MAX_STEPS ){
    heartState = 0;                                     // стадии кончились? Всё с начала.
    heartWait = WAIT_CYCLE;                             // но ожидание начала дольше
  }

  analogWrite(pinHeart, heartLight);                    // изменяем ШИМ (яркость)

}

// ======== Остаток типового скетча: настройки в setup() и повтор действий КА в loop()
void setup() {
  pinMode(pinLed, OUTPUT);      // контрольная 13 нога (встроенный светодиод)
  pwmSet(pinHeart);             // включаем 6-ю ногу в режим ШИМ (PWM) и на выход

  // начальные установки КА: всё в нули, поставят сами при первом вызове
  blinkWait = blinkState = heartWait = heartState = 0;
  heartLight = steps[0];
}

void loop() {
  // пример КА, реализуемый непосредственно внутри вызова макроса:
  everyOVF(blinkWait,
  {
    if( blinkState == 0 ) { blinkState = 1; blinkWait = WAIT_ON; }
    else                  { blinkState = 0; blinkWait = WAIT_OFF; }
    digitalWrite(pinLed, blinkState);
  });

  // отдельную функцию КА прописываем как вызов прямо в параметрах, ибо подстановка текста "как есть":
  everyOVF(heartWait, doHeart() );
}