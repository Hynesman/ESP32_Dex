/**
 * \mainpage Etek ET6621S
 
 *
 * \section sec_dispmem Display memory
 * The RAM is organized into an array of 32 cells of 4 bits each as illustrated by Figure 1.
 * The content of the RAM is directly mapped to the content of the LCD driver and may be affected by the
 * commands \c READ, \c WRITE, \c READ-MODIFY-WRITE.
 *
 * \image latex TM1621Dmem.png "Figure 1. Memory structure of the TM1621D. Remark that we have restructure the adresses so to be able to take care of a full digit."
 * \image html TM1621Dmem.png "Figure 1. Memory structure of the TM1621D. Remark that we have restructure the adresses so to be able to take care of a full digit."
 *
 * \section sec_osc System oscillator
 * The system clock is used in a variety of tasks included LCD driving clock and tone frequency generation.
 * TM1621D has a built-in RC oscillator (256kHZ) and a crystal oscillator (32768Hz). An external 256KHz
 * oscillator can also be used. The type of oscillator used can be selecting by issuing \c XTAL32K,
 * \c RC256K or \c EXT256K commands.
 *
 * Issuing a \c SYS_DIS command stops the system clock, the LCD bias generator and the time base/WDT.
 * The LCD screen will appear blank.
 *
 * An \c LCD_OFF command will turn off the bias generator but not the system clock. Issuing a
 * \c SYS_DIS command will power down the system reducing power consumption.
 *
 * \warning The \c SYS_DIS command affects the system only when the internal RC 256kHZ or the
 * 32.768kHz crystal clocks are used.
 *
 * \section sec_tone Tone output
 * The TM1621D can provide a tone output at the pins \c BZ and \c \f$\bar{BZ}\f$. There are only two tone
 * frequencies, namely: 2kHz and 4kHz that can be selected using the \c TONE2K or \c TONE4K, respectively.
 * Then, the buzzer (if connected to \c BZ and \c \f$\bar{BZ}\f$) start playing at the
 * \c TONE_ON command. Sound is stopped issuing the \c TONE_OFF command.
 *
 * \section sec_powon Power-on
 * At power-on the system is in \c SYS_DIS state. A generic LCD initialization sequence will consist of the following steps:
 * - System Enable
 * - Oscillator configuration
 * - Bias/com configuration
 * - Bias generator start (LCD_ON)
 *
 * \section sec_history History
 * \subsection subsec_v1_0 Version 1.0
 * This the first public version.
 *
 * \section sec_todo Todo list
 * - Improve the overall documentation.
 * - Optimize delays in both writing and reading functions
 * - Overload \c TM1621D::sendCommand() function to send several commands in a row
 * - Test reading functions which use the internal RAM of TM1621D not the simulated RAM.
 */

/**
 * \file ET6621S.h
 * \brief A class for dealing with the Etek ET6621S chip.
 * \author Enrico Formenti
 * \date 31 january 2015
 * \version 1.0
 * \copyright BSD license, check the License page on the blog for more information. All this text must be
 *  included in any redistribution.
 *  <br><br>
 *  See macduino.blogspot.com for more details.
 *
 */

#ifndef _ET6621S_h
#define _ET6621S_h

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define TAKE_CS()    digitalWrite(_CS_pin, HIGH)
#define RELEASE_CS() digitalWrite(_CS_pin, LOW)


// Uncomment the line below if you can read from the TM1621D directly
// #define __TM1621D_READ


class ET6621S {
private:
    uint8_t _CS_pin;
    uint8_t _DATA_pin;
    uint8_t _RW_pin;

protected:
    
#ifndef __TM1621D_READ
    /**
     * \var ram[16]
     * This array is used to simulate the TM1621D internal ram whenever the read operations are not possible.
     * \warning Define the label __TM1621D to disable this feature and use standard read procedures.
     */
    uint8_t ram[16];
#endif

public:
    /*!
     * \enum Modes
     * Operating modes for the ET6621S.
     */
    enum Modes : uint8_t {
        COMMAND_MODE = 0b10000000, /*!< This is used for sending standard commands. */
        READ_MODE = 0b11000000, /*!< This instructs the ET6621S to prepare for reading the internal RAM. */
        WRITE_MODE = 0b10100000, /*!< This instructs the ET6621S to prepare for writing the internal RAM. */
        READ_MODIFY_WRITE_MODE = 0b10100000, /*!< This instructs the ET6621S to prepare for reading/modifying batch of internal RAM adresses. */
        SPECIAL_MODE = 0b10010000 /*!< This instructs the ET6621S to prepare for executing a special command. */
    };
    
    /*!
     * \enum Commands
     *
     * This is an enum of available commands for the ET6621S.
     *
     */
    enum Commands : uint16_t {
        SYS_DIS   = 0b000000000, /*!< System disable. It stops the bias generator and the system oscillator. */
        SYS_EN    = 0b000000010, /*!< System enable. It starts the bias generator and the system oscillator. */
        NORMAL    = 0b111000110, /*!<  */
        LCD_OFF   = 0b000000100, /*!< Turn off the bias generator. */
        LCD_ON    = 0b000000110, /*!< Turn on the bias generator. */
        TIMER_DIS = 0b000001000, /*!< Disable time base output. */
        WDT_DIS   = 0b000001010, /*!< Watch-dog timer disable. */
        TIMER_EN  = 0b000001100, /*!< Enable time base output. */
        WDT_EN    = 0b000001110, /*!< Watch-dog timer enable. The timer is reset. */
        CLR_TIMER = 0b000011000, /*!< Clear the contents of the time base generator. */
        CLR_WDT   = 0b000011100, /*!< Clear the contents of the watch-dog stage. */

        TONE_OFF  = 0b00010000, /*!< Stop emitting the tone signal at the tone pin. \sa TONE2K, TONE4K */
        TONE_ON   = 0b00010010, /*!< Start emitting tone signal at the tone pin. Tone frequency is selected using commands TONE2K or TONE4K. \sa TONE2K, TONE4K */
        TONE2K    = 0b11000000, /*!< Output tone is at 2kHz. */
        TONE4K    = 0b10000000, /*!< Output tone is at 4kHz. */
        
        RC256K    = 0b000110000, /*!< System oscillator is the internal RC oscillator at 256kHz. */
        XTAL32K   = 0b000101000, /*!< System oscillator is the crystal oscillator at 32768Hz. */
        EXT256K   = 0b000111000, /*!< System oscillator is an external oscillator at 256kHz. */
        
        //Set bias to 1/2 or 1/3 cycle
        //Set to 2,3 or 4 connected COM lines
        BIAS_HALF_2_COM  = 0b001000000, /*!< Use 1/2 bias and 2 commons. */
        BIAS_HALF_3_COM  = 0b001001000, /*!< Use 1/2 bias and 3 commons. */
        BIAS_HALF_4_COM  = 0b001010000, /*!< Use 1/2 bias and 4 commons. */
        BIAS_THIRD_2_COM = 0b001000010, /*!< Use 1/3 bias and 2 commons. */
        BIAS_THIRD_3_COM = 0b001001010, /*!< Use 1/3 bias and 3 commons. */
        BIAS_THIRD_4_COM = 0b001010010, /*!< Use 1/3 bias and 4 commons. */
        
        IRQ_EN    = 0b00010000, /*!< Enables IRQ output. This needs to be excuted in SPECIAL_MODE. */
        IRQ_DIS   = 0b00010000, /*!< Disables IRQ output. This needs to be excuted in SPECIAL_MODE. */
        
        // WDT configuration commands
        F1 = 0b01000000, /*!< Time base/WDT clock. Output = 1Hz. Time-out = 4s. This needs to be excuted in SPECIAL_MODE. */
        F2 = 0b01000010, /*!< Time base/WDT clock. Output = 2Hz. Time-out = 2s. This needs to be excuted in SPECIAL_MODE. */
        F4 = 0b01000100, /*!< Time base/WDT clock. Output = 4Hz. Time-out = 1s. This needs to be excuted in SPECIAL_MODE. */
        F8 = 0b01000110, /*!< Time base/WDT clock. Output = 8Hz. Time-out = .5s. This needs to be excuted in SPECIAL_MODE. */
        F16 = 0b01001000, /*!< Time base/WDT clock. Output = 16Hz. Time-out = .25s. This needs to be excuted in SPECIAL_MODE. */
        F32 = 0b01001010, /*!< Time base/WDT clock. Output = 32Hz. Time-out = .125s. This needs to be excuted in SPECIAL_MODE. */
        F64 = 0b01001100, /*!< Time base/WDT clock. Output = 64Hz. Time-out = .0625s. This needs to be excuted in SPECIAL_MODE. */
        F128 = 0b01001110, /*!< Time base/WDT clock. Output = 128Hz. Time-out = .03125s. This needs to be excuted in SPECIAL_MODE. */
        
        //Don't use
        TEST_ON   = 0b11000000, /*!< Don't use! Only for manufacturers. This needs SPECIAL_MODE. */
        TEST_OFF  = 0b11000110  /*!< Don't use! Only for manufacturers. This needs SPECIAL_MODE. */
    };

    bool UpsideDown;

    // /*!
    //  * \enum Brightness
    //  *
    //  * This is an enum of commands to alter brightness
    //  *
    //  */
    // enum Brightness : uint32_t {
    //   PROJ_BRIGHT = 0b11011110101111101111100100000000,
    //   PROJ_DIMMED = 0b00000000000001101011111111011110
    // };

    enum Character : uint8_t {
      BLANK = 10,
      LETTER_F,
      DASH,
      DEGREE
    };


    /**
     * \fn ET6621S(uint8_t CSpin, uint8_t RWpin, uint8_t DATApin)
     * \brief Constructor. Use begin() to complete the initialization of the chip.
     * @param \c CSpin Channel select pin.
     * @param \c RWpin Read/Write signal pin
     * @param \c DATApin Data pin both for reading or writing data.
     */
    ET6621S(uint8_t CSpin, uint8_t RWpin, uint8_t DATApin) : _CS_pin(CSpin), _DATA_pin(DATApin), _RW_pin(RWpin) {};

    /**
     * \fn void begin(void)
     * \brief Init the ET6621S. It inits the control bus. Moreover, it clears the (simulated) ram if \c __ET6621S_READ is defined.
     */
    void begin(void);

    /**
     * \fn void start(void)
     * \brief Start the ET6621S. this means sending the startup sequences to config the projector
     */
    void start(void);

    /**
     * \fn void projector_lamp(bool)
     * \brief Turn the projector lamp on or off based on the parameter.
     * @param on Boolean to indicate projector lamp is to be turned on or off.
     */
    void projector_lamp(bool on);

    /**
     * \fn void writeBits(uint8_t data, uint16_t mask, uint8_t cnt)
     * \brief Send bits to the ET6621S.
     * @param data Data to be sent to the ET6621S seen as an array of bits.
     * @param cnt Number of bits to send to the ET6621S.
     * \warning In order to allow series of data to be sent CS is not taken by this function, so several writes can be issued in a row. You need to control CS by your own.
     * \warning There is no check on the size of cnt. Hence avoid ask to send more than 7 bits per time.
     * \sa readBits()
     */
    void writeBits(uint16_t data, uint16_t mask, uint8_t cnt);

    /**
     * \fn uint8_t readBits(uint8_t cnt)
     * \brief Reads bits from the ET6621S.
     * @param cnt Number of bits to read. Maximal number of bits that can be read is 8.
     * \return uint8_t A byte containing the bits read. Last bit is the leftmost one.
     * \warning There is no check if too much bits are read. Only the last batch of 8 will be sent back.
     * \sa writeBits()
     */
    void sendCommand(uint16_t cmd, bool first = true, bool last = true);

    /** 
     * \fn void write(uint8_t address, uint8_t data)
     * \brief Write \c data at the given address.
     * @param address Address to which write the data. Max address is 128.
     * @param data Data to be written. Remark that only the 4 less significative bits are used.
     * \warning There is no check to verify if the address is valid.
     */
    void write(uint8_t address, uint8_t data);

    /** 
     * \fn void write(uint8_t address, uint8_t data, uint8_t cnt)
     * \brief Write \c data at the given address and at the \c cnt successive addresses.
     * @param address Address to which write the data. Max address is 128.
     * @param data Data to be written. Remark that only the 4 less significative bits are used.
     * @param cnt Number of times that \c data has to be written
     * \warning There is no check to verify if the address is valid. Moreover, pay attention that 
     * the address \c (address+cnt) has also to be valid.
     */

    void write(uint8_t address, uint8_t data, uint8_t cnt);

    /**
     * \fn void write(uint8_t address, uint8_t *data, uint8_t cnt)
     * \brief Write \c cnt bytes starting at \c address and take data from buffer \c data.
     * @param address Address to which start writing data. Max address is 128.
     * @param data Buffer to be written.
     * @param cnt Length of the buffer.
     * \warning The buffer is byte aligned, so it is not very efficient. Indeed, only the 4 less significant
     * bits are written.
     * \warning There is no check that the buffer is of suitable length.
     */
    void write_long(uint8_t address, uint32_t data);

    /**
     *  \fn void all_elements_on(void)
     *  \brief Turn on all of the LCD projector elements as a test.
     */
    void all_elements_on(void);

    /**
     *  \fn void all_elements_off(void)
     *  \brief Turn off all projector elements as a screen clear type function.
     */
    void all_elements_off(void);

    /**
     *  \fn void display_test(void)
     *  \brief Run thru all characters, respecting orientation. Also test symbols (colon & PM's)
     */
    void display_test(void);

    /**
     * \fn display_numeral(uint8_t pos, uint8_t digit, bool colon=false, bool am=false, bool pm=false)
     * \brief Generate a character in a position on the projector display
     * @param \c pos Position of the numeral. 0 - 3. If display is upside down, these numbers are reversed.
     * @param \c digit Index into array defining the segments to turn on for a character
     * @param \c colon If TRUE, turn on colon character. If FALSE, turn it off. Default to false.
     * @param \c am If TRUE, turn on upper/lower "AM" symbol. Default to false.
     * @param \c pm If TRUE, turn on upper/lower "PM" symbol. Default to false.
     */
    void display_numeral(uint8_t pos, uint8_t digit, bool colon=false, bool am=false, bool pm=false);

    /**
     *  \fn void set_orientation(bool)
     *  \brief Set the orientation of the display. True means the display is upside down.
     */
    void set_orientation(bool orientation);

};

#endif
