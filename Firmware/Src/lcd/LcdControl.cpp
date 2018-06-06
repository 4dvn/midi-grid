#include "lcd/LcdControl.h"

namespace lcd_control
{

static DMA_HandleTypeDef lcdSpiDma;

extern "C" void DMA1_Stream4_IRQHandler()
{
    HAL_DMA_IRQHandler(&lcdSpiDma);
}

LcdControl::LcdControl()
{
}

LcdControl::~LcdControl()
{
}

void LcdControl::initialize()
{
    initializeGpio();
    initializeSpi();
    initializeDma();
    //initializeBacklightPwm();

    resetController();
    writeCommand( 0x21 ); // LCD extended commands.
    writeCommand( 0xB8 ); // set LCD Vop(Contrast).
    writeCommand( 0x04 ); // set temp coefficent.
    writeCommand( 0x15 ); // LCD bias mode 1:65. used to be 0x14 - 1:40
    writeCommand( 0x20 ); // LCD basic commands.
    writeCommand( 0x0C ); // LCD normal.
}

void LcdControl::setBacklightIntensity( uint8_t intensity )
{
    if (intensity >= NUMBER_OF_BACKLIGHT_INTENSITY_LEVELS)
    {
        intensity = NUMBER_OF_BACKLIGHT_INTENSITY_LEVELS - 1;
    }

    BACKLIGHT_TIMER->CCR2 = backlightIntensity[intensity]; // would be appropriate to use hal function here
}

void LcdControl::transmit( uint8_t* buffer )
{
    setCursor( 0, 0 );

    while (HAL_DMA_STATE_BUSY == HAL_DMA_GetState( &lcdSpiDma ))
    {
        // wait until previous transfer is done
    }

    HAL_GPIO_WritePin( LCD_GPIO_Port, DC_Pin, GPIO_PIN_SET );  //data mode
    HAL_SPI_Transmit_DMA( &lcdSpi_, buffer, BUFFER_SIZE );
}

void LcdControl::initializeBacklight()
{
    TIM_ClockConfigTypeDef timerClockSourceConfiguration;
    TIM_OC_InitTypeDef timerOutputCompareConfiguration;

    __HAL_RCC_TIM1_CLK_ENABLE();

    // prescaler and period of PWM timer are calculated based on period of base timer
    lcdBacklightPwmTimer_.Instance = BACKLIGHT_TIMER;
    lcdBacklightPwmTimer_.Init.Prescaler = 16 - 1; // ~100Hz
    lcdBacklightPwmTimer_.Init.CounterMode = TIM_COUNTERMODE_UP;
    lcdBacklightPwmTimer_.Init.Period = 65535 - 1;
    lcdBacklightPwmTimer_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init( &lcdBacklightPwmTimer_ );

    timerClockSourceConfiguration.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource( &lcdBacklightPwmTimer_, &timerClockSourceConfiguration );

    HAL_TIM_PWM_Init( &lcdBacklightPwmTimer_ );

    timerOutputCompareConfiguration.OCMode = TIM_OCMODE_PWM1;
    timerOutputCompareConfiguration.Pulse = 0;
    timerOutputCompareConfiguration.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    timerOutputCompareConfiguration.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    timerOutputCompareConfiguration.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel( &lcdBacklightPwmTimer_, &timerOutputCompareConfiguration, TIM_CHANNEL_2 );

    HAL_TIMEx_PWMN_Start( &lcdBacklightPwmTimer_, TIM_CHANNEL_2 );
}

void LcdControl::initializeDma()
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    // SPI2 DMA Init
    // SPI2_TX Init
    lcdSpiDma.Instance = DMA1_Stream4;
    lcdSpiDma.Init.Channel = DMA_CHANNEL_0;
    lcdSpiDma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    lcdSpiDma.Init.PeriphInc = DMA_PINC_DISABLE;
    lcdSpiDma.Init.MemInc = DMA_MINC_ENABLE;
    lcdSpiDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    lcdSpiDma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    lcdSpiDma.Init.Mode = DMA_NORMAL;
    lcdSpiDma.Init.Priority = DMA_PRIORITY_LOW;
    lcdSpiDma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init( &lcdSpiDma );

    __HAL_LINKDMA( &lcdSpi_, hdmatx, lcdSpiDma );

    // DMA interrupt init
    // DMA1_Stream4_IRQn interrupt configuration
    HAL_NVIC_SetPriority( DMA1_Stream4_IRQn, 0, 0 );
    HAL_NVIC_EnableIRQ( DMA1_Stream4_IRQn );
}

void LcdControl::initializeGpio()
{
    GPIO_InitTypeDef gpioConfiguration;

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpioConfiguration.Pin = DC_Pin | RESET_Pin;
    gpioConfiguration.Mode = GPIO_MODE_OUTPUT_PP;
    gpioConfiguration.Pull = GPIO_NOPULL;
    gpioConfiguration.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init( LCD_GPIO_Port, &gpioConfiguration );

    gpioConfiguration.Pin = CS_Pin | SCK_Pin | MOSI_Pin;
    gpioConfiguration.Mode = GPIO_MODE_AF_PP;
    gpioConfiguration.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init( LCD_GPIO_Port, &gpioConfiguration );

    gpioConfiguration.Pin = LIGHT_Pin;
    gpioConfiguration.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init( LCD_GPIO_Port, &gpioConfiguration );
}

void LcdControl::initializeSpi()
{
    __HAL_RCC_SPI2_CLK_ENABLE();

    // SPI2 parameter configuration
    lcdSpi_.Instance = SPI2;
    lcdSpi_.Init.Mode = SPI_MODE_MASTER;
    lcdSpi_.Init.Direction = SPI_DIRECTION_2LINES;
    lcdSpi_.Init.DataSize = SPI_DATASIZE_8BIT;
    lcdSpi_.Init.CLKPolarity = SPI_POLARITY_LOW;
    lcdSpi_.Init.CLKPhase = SPI_PHASE_1EDGE;
    lcdSpi_.Init.NSS = SPI_NSS_HARD_OUTPUT;
    lcdSpi_.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32; // 3MBbit?
    lcdSpi_.Init.FirstBit = SPI_FIRSTBIT_MSB;
    lcdSpi_.Init.TIMode = SPI_TIMODE_DISABLE;
    lcdSpi_.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init( &lcdSpi_ );
}

void LcdControl::resetController()
{
    HAL_GPIO_WritePin( LCD_GPIO_Port, RESET_Pin, GPIO_PIN_RESET );
    HAL_GPIO_WritePin( LCD_GPIO_Port, RESET_Pin, GPIO_PIN_SET );
}

void LcdControl::setCursor( const uint8_t x, const uint8_t y )
{
    writeCommand( 0x80 | x ); // column.
    writeCommand( 0x40 | y ); // row.
}

void LcdControl::writeCommand( const uint8_t command )
{
    uint8_t commandToWrite = command;

    while (HAL_DMA_STATE_BUSY == HAL_DMA_GetState( &lcdSpiDma ))
    {
        // wait until previous transfer is done
    }

    HAL_GPIO_WritePin( LCD_GPIO_Port, DC_Pin, GPIO_PIN_RESET ); //command mode
    HAL_SPI_Transmit_DMA( &lcdSpi_, &commandToWrite, 1 );
}

} // namespace

