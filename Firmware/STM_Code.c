#include "main.h"
#include "i2c.h"
#include "gpio.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    char c;
    uint8_t col[5];
} FontChar;

// I2C Addresses
#define MPU6050_ADDR   (0x68 << 1)
#define OLED_ADDR      (0x3C << 1)

#define REG_PWR_MGMT_1   0x6B
#define REG_ACCEL_CONFIG 0x1C
#define REG_ACCEL_XOUT_H 0x3B

#define SAMPLE_COUNT      1000
#define SAMPLE_INTERVAL   1
#define GRAVITY           9.81f

#define OLED_WIDTH  128

static const FontChar font[] = {
    {'0', {0x3E,0x51,0x49,0x45,0x3E}},
    {'1', {0x00,0x42,0x7F,0x40,0x00}},
    {'2', {0x42,0x61,0x51,0x49,0x46}},
    {'3', {0x21,0x41,0x45,0x4B,0x31}},
    {'4', {0x18,0x14,0x12,0x7F,0x10}},
    {'5', {0x27,0x45,0x45,0x45,0x39}},
    {'6', {0x3C,0x4A,0x49,0x49,0x30}},
    {'7', {0x01,0x71,0x09,0x05,0x03}},
    {'8', {0x36,0x49,0x49,0x49,0x36}},
    {'9', {0x06,0x49,0x49,0x29,0x1E}},
    {' ', {0x00,0x00,0x00,0x00,0x00}},
    {'.', {0x00,0x60,0x60,0x00,0x00}},
    {'-', {0x08,0x08,0x08,0x08,0x08}},
    {'/', {0x20,0x10,0x08,0x04,0x02}},
    {'A', {0x7E,0x09,0x09,0x09,0x7E}},
    {'B', {0x7F,0x49,0x49,0x49,0x36}},
    {'C', {0x3E,0x41,0x41,0x41,0x22}},
    {'D', {0x7F,0x41,0x41,0x41,0x3E}},
    {'E', {0x7F,0x49,0x49,0x49,0x41}},
    {'F', {0x7F,0x09,0x09,0x09,0x01}},
    {'G', {0x3E,0x41,0x49,0x49,0x7A}},
    {'H', {0x7F,0x08,0x08,0x08,0x7F}},
    {'I', {0x00,0x41,0x7F,0x41,0x00}},
    {'K', {0x7F,0x08,0x14,0x22,0x41}},
    {'L', {0x7F,0x40,0x40,0x40,0x40}},
    {'M', {0x7F,0x02,0x0C,0x02,0x7F}},
    {'N', {0x7F,0x04,0x08,0x10,0x7F}},
    {'O', {0x3E,0x41,0x41,0x41,0x3E}},
    {'P', {0x7F,0x09,0x09,0x09,0x06}},
    {'R', {0x7F,0x09,0x19,0x29,0x46}},
    {'S', {0x46,0x49,0x49,0x49,0x31}},
    {'T', {0x01,0x01,0x7F,0x01,0x01}},
    {'U', {0x3F,0x40,0x40,0x40,0x3F}},
    {'Y', {0x07,0x08,0x70,0x08,0x07}},
    {'a', {0x20,0x54,0x54,0x54,0x78}},
    {'b', {0x7F,0x48,0x44,0x44,0x38}},
    {'c', {0x38,0x44,0x44,0x44,0x20}},
    {'d', {0x38,0x44,0x44,0x48,0x7F}},
    {'e', {0x38,0x54,0x54,0x54,0x18}},
    {'h', {0x7F,0x08,0x04,0x04,0x78}},
    {'i', {0x00,0x44,0x7D,0x40,0x00}},
    {'k', {0x7F,0x10,0x28,0x44,0x00}},
    {'m', {0x7C,0x04,0x18,0x04,0x78}},
    {'n', {0x7C,0x08,0x04,0x04,0x78}},
    {'o', {0x38,0x44,0x44,0x44,0x38}},
    {'p', {0x7C,0x14,0x14,0x14,0x08}},
    {'r', {0x7C,0x08,0x04,0x04,0x08}},
    {'s', {0x48,0x54,0x54,0x54,0x20}},
    {'t', {0x04,0x3F,0x44,0x40,0x20}},
    {'u', {0x3C,0x40,0x40,0x20,0x7C}},
    {'z', {0x44,0x64,0x54,0x4C,0x44}},
};
#define FONT_SIZE (sizeof(font)/sizeof(font[0]))

char textBuf[32];
float samples[SAMPLE_COUNT];

void SystemClock_Config(void);

void OLED_Command(uint8_t cmd);
void OLED_Data(uint8_t *data, uint16_t size);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetCursor(uint8_t col, uint8_t page);
void OLED_WriteChar(uint8_t col, uint8_t page, char c);
void OLED_WriteChar2X(uint8_t col, uint8_t page, char c);
void OLED_Print(uint8_t col, uint8_t page, char *str);
void OLED_PrintCentered(uint8_t page, char *str);
void OLED_PrintCentered2X(uint8_t page, char *str);

int main(void)
{
 
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();

  HAL_Delay(100);

  // Wake up MPU6050 (try multiple times)
  uint8_t data[2];
  for (int attempt = 0; attempt < 3; attempt++)
  {
      data[0] = REG_PWR_MGMT_1;
      data[1] = 0x00;
      if (HAL_I2C_Master_Transmit(&hi2c1, MPU6050_ADDR, data, 2, 100) == HAL_OK)
          break;
      HAL_Delay(50);
  }
  HAL_Delay(50);

  // Set MPU6050 to ±16g range
  data[0] = REG_ACCEL_CONFIG;
  data[1] = 0x18;
  HAL_I2C_Master_Transmit(&hi2c1, MPU6050_ADDR, data, 2, 100);
  HAL_Delay(10);

  // Initialize OLED
  OLED_Init();
  OLED_Clear();

  // Show idle screen
  OLED_PrintCentered2X(0, "PRESS");
  OLED_PrintCentered2X(2, "START");
  OLED_PrintCentered(5, "Punch on 3rd buzz!");

  while (1)
  {
    // Wait for button press (PA1, active low)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET)
    {
        HAL_Delay(50);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) != GPIO_PIN_RESET) continue;

        // ========== COUNTDOWN ==========

        // Buzz 1
        OLED_Clear();
        OLED_PrintCentered2X(2, "3");
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_Delay(250);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_Delay(500);

        // Buzz 2
        OLED_Clear();
        OLED_PrintCentered2X(2, "2");
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_Delay(250);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_Delay(500);

        // Buzz 3
        OLED_Clear();
        OLED_PrintCentered2X(2, "1");
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_Delay(250);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

        // ========== CAPTURE SAMPLES ==========

        OLED_Clear();
        OLED_PrintCentered2X(2, "GO!");

        uint32_t nextSampleTime = HAL_GetTick();

        for (int i = 0; i < SAMPLE_COUNT; i++)
        {
            while (HAL_GetTick() < nextSampleTime) {}

            // Read X acceleration
            uint8_t reg = REG_ACCEL_XOUT_H;
            uint8_t buf[2];
            HAL_I2C_Master_Transmit(&hi2c1, MPU6050_ADDR, &reg, 1, HAL_MAX_DELAY);
            HAL_I2C_Master_Receive(&hi2c1, MPU6050_ADDR, buf, 2, HAL_MAX_DELAY);

            int16_t rawX = (int16_t)((buf[0] << 8) | buf[1]);
            samples[i] = -((float)rawX / 2048.0f);  // Flip sign, convert to g

            nextSampleTime += SAMPLE_INTERVAL;
        }

        // ========== CALCULATE SPEED ==========

        // Only use positive acceleration values
        float sumAccel = 0.0f;
        int count = 0;

        for (int i = 0; i < SAMPLE_COUNT; i++)
        {
            if (samples[i] > 0)
            {
                sumAccel += samples[i];
                count++;
            }
        }

        // Calculate velocity
        float speedKmh = 0.0f;
        if (count > 0)
        {
            float avgAccel = sumAccel / count;
            float totalTime = count * 0.001f;
            float velocity = avgAccel * GRAVITY * totalTime;
            speedKmh = velocity * 3.6f;
        }

        // ========== DISPLAY RESULT ==========

        OLED_Clear();
        OLED_PrintCentered(0, "Punch Speed");

        int speedInt = (int)speedKmh;
        int speedDec = (int)((speedKmh - speedInt) * 10);
        if (speedDec < 0) speedDec = -speedDec;
        snprintf(textBuf, sizeof(textBuf), "%d.%d", speedInt, speedDec);
        OLED_PrintCentered2X(2, textBuf);

        OLED_PrintCentered2X(5, "km/h");

        HAL_Delay(5000);

        // ========== BACK TO IDLE ==========

        OLED_Clear();
        OLED_PrintCentered2X(0, "PRESS");
        OLED_PrintCentered2X(2, "START");
        OLED_PrintCentered(5, "Punch on 3rd buzz!");

        while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET)
        {
            HAL_Delay(10);
        }
    }

    HAL_Delay(20);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

// =================== OLED FUNCTIONS ===================

void OLED_Command(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, data, 2, HAL_MAX_DELAY);
}

void OLED_Data(uint8_t *data, uint16_t size)
{
    uint8_t buffer[17];
    buffer[0] = 0x40;
    while (size > 0)
    {
        uint8_t chunk = (size > 16) ? 16 : size;
        memcpy(&buffer[1], data, chunk);
        HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, buffer, chunk + 1, HAL_MAX_DELAY);
        data += chunk;
        size -= chunk;
    }
}

void OLED_Init(void)
{
    HAL_Delay(100);
    OLED_Command(0xAE);  // Display off
    OLED_Command(0x20); OLED_Command(0x00);  // Horizontal addressing
    OLED_Command(0xB0);  // Page 0
    OLED_Command(0xC8);  // COM scan direction
    OLED_Command(0x00); OLED_Command(0x10);  // Column 0
    OLED_Command(0x40);  // Start line 0
    OLED_Command(0x81); OLED_Command(0xFF);  // Max contrast
    OLED_Command(0xA1);  // Segment remap
    OLED_Command(0xA6);  // Normal display
    OLED_Command(0xA8); OLED_Command(0x3F);  // Multiplex 64
    OLED_Command(0xA4);  // Display from RAM
    OLED_Command(0xD3); OLED_Command(0x00);  // No offset
    OLED_Command(0xD5); OLED_Command(0x80);  // Clock
    OLED_Command(0xD9); OLED_Command(0xF1);  // Pre-charge
    OLED_Command(0xDA); OLED_Command(0x12);  // COM pins
    OLED_Command(0xDB); OLED_Command(0x40);  // VCOMH
    OLED_Command(0x8D); OLED_Command(0x14);  // Charge pump on
    OLED_Command(0xAF);  // Display on
}

void OLED_SetCursor(uint8_t col, uint8_t page)
{
    OLED_Command(0xB0 | (page & 0x07));
    OLED_Command(0x00 | (col & 0x0F));
    OLED_Command(0x10 | ((col >> 4) & 0x0F));
}

void OLED_Clear(void)
{
    uint8_t zeros[128] = {0};
    for (uint8_t page = 0; page < 8; page++)
    {
        OLED_SetCursor(0, page);
        OLED_Data(zeros, 128);
    }
}

void OLED_WriteChar(uint8_t col, uint8_t page, char c)
{
    uint8_t buf[6] = {0, 0, 0, 0, 0, 0};

    for (int i = 0; i < FONT_SIZE; i++)
    {
        if (font[i].c == c)
        {
            for (int j = 0; j < 5; j++) buf[j] = font[i].col[j];
            break;
        }
    }

    OLED_SetCursor(col, page);
    OLED_Data(buf, 6);
}

void OLED_WriteChar2X(uint8_t col, uint8_t page, char c)
{
    uint8_t bufTop[12] = {0};
    uint8_t bufBot[12] = {0};

    // Find character in font
    const uint8_t *cols = NULL;
    for (int i = 0; i < FONT_SIZE; i++)
    {
        if (font[i].c == c)
        {
            cols = font[i].col;
            break;
        }
    }
    if (cols == NULL) return;

    // Scale 2x vertically and horizontally
    for (int i = 0; i < 5; i++)
    {
        uint16_t scaled = 0;
        for (int bit = 0; bit < 8; bit++)
        {
            if (cols[i] & (1 << bit))
                scaled |= (3 << (bit * 2));
        }
        bufTop[i*2] = bufTop[i*2+1] = scaled & 0xFF;
        bufBot[i*2] = bufBot[i*2+1] = (scaled >> 8) & 0xFF;
    }

    OLED_SetCursor(col, page);
    OLED_Data(bufTop, 12);
    OLED_SetCursor(col, page + 1);
    OLED_Data(bufBot, 12);
}

void OLED_Print(uint8_t col, uint8_t page, char *str)
{
    while (*str)
    {
        OLED_WriteChar(col, page, *str++);
        col += 6;
        if (col > 122) break;
    }
}

void OLED_PrintCentered(uint8_t page, char *str)
{
    uint8_t len = strlen(str);
    uint8_t col = (OLED_WIDTH - len * 6) / 2;
    OLED_Print(col, page, str);
}

void OLED_PrintCentered2X(uint8_t page, char *str)
{
    uint8_t len = strlen(str);
    uint8_t col = (OLED_WIDTH - len * 12) / 2;
    while (*str)
    {
        OLED_WriteChar2X(col, page, *str++);
        col += 12;
        if (col > 116) break;
    }
}

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
