#include <Arduino.h>

// Definição de Macros
#define set_bit(Y, bit_x) (Y |= (1 << bit_x))  // ativa o bit x da variável
#define clr_bit(Y, bit_x) (Y &= ~(1 << bit_x)) // limpa o bit x da variável
#define tst_bit(Y, bit_x) (Y & (1 << bit_x))   // testa o bit x da variável
#define cpl_bit(Y, bit_x) (Y ^= (1 << bit_x))  // troca o estado do bit x

// Definição de pinos
#define BTN_UP_PIN A3
#define BTN_DOWN_PIN A2
#define BTN_CONF_PIN A1
#define POT_PIN A0
#define BUZZER_PIN 7

// Constantes para debounce
#define DEBOUNCE_DELAY 50
#define LONG_PRESS_TIME 5000

// Definição de enums
enum MENU_STATE
{
  OFF,
  TIME_CONFIG,
  ALARM_CONFIG,
  SELECT,
};

// Variáveis globais
unsigned long previousMillis = 0;                       // Para controle de tempo
unsigned long instantPressed = 0;                       // Tempo do botão pressionado
unsigned int menuState = MENU_STATE::OFF;               // Estado atual do menu
unsigned int selectNextState = MENU_STATE::TIME_CONFIG; // Estado a ser selecionado durante o select do menu
bool stopAlarm = true;
int hours = 0;             // Horas
int minutes = 0;           // Minutos
int alarmHour = 0;         // Hora do despertador
int alarmMinute = 0;       // Minuto do despertador
int buzzerFrequency = 100; // Frequência do buzzer

/** Array de bytes com a sequência para formar todos os números possíveis.*/
byte numBytes[10] = {
    B00111111, // 0
    B00000110, // 1
    B01011011, // 2
    B01001111, // 3
    B01100110, // 4
    B01101101, // 5
    B01111101, // 6
    B00000111, // 7
    B01111111, // 8
    B01101111  // 9
};

/**
 * Função para verificar o estado dos botões com debounce
 */
bool isButtonPressed(int buttonPin)
{
  return digitalRead(buttonPin) == LOW;
}

/**
 * Função para atualizar o horário
 */
void updateClock()
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 60000)
  { // Passou 1 minuto
    previousMillis = currentMillis;

    minutes++;

    if (minutes >= 60)
    {
      minutes = 0;
      hours++;

      if (hours >= 24)
      {
        hours = 0;
      }
    }
  }
}

void updateBuzzerTone()
{
  buzzerFrequency = map(analogRead(POT_PIN), 0, 1023, 0, 20000);
}

/**
 * Atualiza todos os displays do painel.
 */
void updateDisplays(int digits[4])
{
  for (int display = 0; display < 4; display++)
  {
    PORTB = PORTB & B11110000;         // Bota pra LOW todos os pinos dos TBJ's
    PORTD = numBytes[digits[display]]; // Seta no PORTD o número a ser impresso
    set_bit(PORTB, display);           // Coloca o TBJ do respectivo display para HIGH para imprimir na tela o número
    delayMicroseconds(1000);
  }
}

void setup()
{
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_CONF_PIN, INPUT_PULLUP);

  DDRB = B00011111; // Setando as portas dos TBJ's
  DDRD = B11111111; // Setando as portas do display e do buzzer
}

void executeMenuOffState()
{
  int digits[4] = {hours / 10, hours % 10, minutes / 10, minutes % 10};
  updateDisplays(digits);
  if (isButtonPressed(BTN_CONF_PIN))
  {
    while (isButtonPressed(BTN_CONF_PIN))
    {
      updateClock();
      int digits[4] = {hours / 10, hours % 10, minutes / 10, minutes % 10};
      updateDisplays(digits);
    }
    menuState = MENU_STATE::SELECT;
  }
}

void executeSelectState()
{
  int digits[4] = {0, 0, 0, selectNextState};
  if (isButtonPressed(BTN_UP_PIN) || isButtonPressed(BTN_DOWN_PIN))
  {
    while (isButtonPressed(BTN_UP_PIN) || isButtonPressed(BTN_DOWN_PIN))
    {
      updateDisplays(digits);
    }
    if (selectNextState == MENU_STATE::TIME_CONFIG)
    {
      selectNextState = MENU_STATE::ALARM_CONFIG;
    }
    else
    {
      selectNextState = MENU_STATE::TIME_CONFIG;
    }
  }

  if (isButtonPressed(BTN_CONF_PIN))
  {
    while (isButtonPressed(BTN_CONF_PIN))
    {
      updateDisplays(digits);
    }
    menuState = selectNextState;
  }

  updateDisplays(digits);
}

void executeTimeConfigState()
{

  if (isButtonPressed(BTN_UP_PIN))
  {
    while (isButtonPressed(BTN_UP_PIN))
    {
      int digits[4] = {hours / 10, hours % 10, minutes / 10, minutes % 10};
      updateDisplays(digits);
    }
    minutes++;
    if (minutes >= 60)
    {
      minutes = 0;
      hours++;
      if (hours >= 24)
      {
        hours = 0;
      }
    }
  }

  if (isButtonPressed(BTN_DOWN_PIN))
  {
    while (isButtonPressed(BTN_DOWN_PIN))
    {
      int digits[4] = {hours / 10, hours % 10, minutes / 10, minutes % 10};
      updateDisplays(digits);
    }
    minutes--;
    if (minutes < 0)
    {
      minutes = 59;
      hours--;
      if (hours < 0)
      {
        hours = 23;
      }
    }
  }

  if (isButtonPressed(BTN_CONF_PIN))
  {
    while (isButtonPressed(BTN_CONF_PIN))
    {
      int digits[4] = {alarmHour / 10, alarmHour % 10, alarmMinute / 10, alarmMinute % 10};
      updateDisplays(digits);
    }
    menuState = MENU_STATE::OFF;
  }

  int digits[4] = {hours / 10, hours % 10, minutes / 10, minutes % 10};
  updateDisplays(digits);
}

void executeAlarmConfigState()
{
  if (isButtonPressed(BTN_UP_PIN))
  {
    while (isButtonPressed(BTN_UP_PIN))
    {
      int digits[4] = {alarmHour / 10, alarmHour % 10, alarmMinute / 10, alarmMinute % 10};
      updateDisplays(digits);
    }
    alarmMinute++;
    if (alarmMinute >= 60)
    {
      alarmMinute = 0;
      alarmHour++;
      if (alarmHour >= 24)
      {
        alarmHour = 0;
      }
    }
  }

  if (isButtonPressed(BTN_DOWN_PIN))
  {
    while (isButtonPressed(BTN_DOWN_PIN))
    {
      int digits[4] = {alarmHour / 10, alarmHour % 10, alarmMinute / 10, alarmMinute % 10};
      updateDisplays(digits);
    }
    alarmMinute--;
    if (alarmMinute < 0)
    {
      alarmMinute = 59;
      alarmHour--;
      if (alarmHour < 0)
      {
        alarmHour = 23;
      }
    }
  }

  if (isButtonPressed(BTN_CONF_PIN))
  {
    while (isButtonPressed(BTN_CONF_PIN))
    {
      int digits[4] = {alarmHour / 10, alarmHour % 10, alarmMinute / 10, alarmMinute % 10};
      updateDisplays(digits);
    }
    menuState = MENU_STATE::OFF;
  }

  int digits[4] = {alarmHour / 10, alarmHour % 10, alarmMinute / 10, alarmMinute % 10};
  updateDisplays(digits);
}

void loop()
{
  switch (menuState)
  {
  case MENU_STATE::SELECT:
    executeSelectState();
    break;

  case MENU_STATE::TIME_CONFIG:
    executeTimeConfigState();
    break;

  case MENU_STATE::ALARM_CONFIG:
    executeAlarmConfigState();
    break;

  default:
    executeMenuOffState();
    break;
  }

  if (menuState != MENU_STATE::TIME_CONFIG)
  {
    updateClock();
  }

  if (hours == alarmHour && minutes == alarmMinute && stopAlarm == false)
  {

    if (isButtonPressed(BTN_UP_PIN))
    {
      while (isButtonPressed(BTN_UP_PIN))
      {
      }
      stopAlarm = true;
    }
    tone(BUZZER_PIN, buzzerFrequency);
  }
  else
  {
    noTone(BUZZER_PIN);
  }

  if (hours != alarmHour || minutes != alarmMinute) {
    stopAlarm = false;
  }

  digitalWrite(12, stopAlarm);
  updateBuzzerTone();
}