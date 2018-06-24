#include "xil_io.h"
#include "ssd1306_driver.h"

/*
 * The below shows layout of accessible data in AXI registers.
 *
 *  -> .slv_reg0(
 *          {{C_S00_AXI_DATA_WIDTH - 4{1'bZ}},
 *          is_din_u8,
 *          is_din_data,
 *          should_send_din,
 *          should_turn_power_on
*       }),
 *  -> .slv_reg1(
 *          din
*       ),
 *  <- .slv_reg2(
 *          {{C_S00_AXI_DATA_WIDTH - 1{1'b0}},
 *          is_busy
*      }),
 */
#define SHOULD_TURN_POWER_ON_MASK   ((u32)(1 << 0))
#define SHOULD_SEND_DIN_MASK        ((u32)(1 << 1))
#define IS_DIN_DATA_MASK            ((u32)(1 << 2))
#define IS_DIN_U8_MASK              ((u32)(1 << 3))
#define IS_BUSY_MASK                ((u32)(1 << 0))


class Ssd1306 {

public:
    
    /*
     * The commands:
     *
     * - Set Lower Column Start Address for Page Addressing Mode (00h~0Fh)
     * - Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
     * - Set Memory Addressing Mode (20h)
     * - Set Column Address (21h)
     * - Set Page Address (22h)
     * - Set Display Start Line (40h~7Fh)
     * - Set Contrast Control for BANK0 (81h)
     * - Set Segment Re-map (A0h/A1h)
     * - Entire Display ON (A4h/A5h)
     * - Set Normal/Inverse Display (A6h/A7h)
     * - Set Multiplex Ratio (A8h)
     * - Set Display ON/OFF (AEh/AFh)
     * - Set Page Start Address for Page Addressing Mode (B0h~B7h)
     * - Set COM Output Scan Direction (C0h/C8h)
     * - Set Display Offset (D3h)
     * - Set Display Clock Divide Ratio/ Oscillator Frequency (D5h)
     * - Set Pre-charge Period (D9h)
     * - Set COM Pins Hardware Configuration (DAh)
     * - Set VCOMH Deselect Level (DBh)
     * - NOP (E3h)
     * - Status register Read - unavailable with serial mode (e.g. SPI on ZedBoard)
     * - Horizontal Scroll Setup (26h/27h)
     * - Continuous Vertical and Horizontal Scroll Setup (29h/2Ah)
     * - Deactivate Scroll (2Eh)
     * - Activate Scroll (2Fh)
     * - Set Vertical Scroll Area(A3h)
     * - Charge Pump (8Dh)
     */
    enum Command {
        ActivateScroll = 0x2F,
        ChargePump = 0x8D,
        ColumnAddress = 0x21,
        ComOutputScanDirectionNormal = 0xC0,
        ComOutputScanDirectionRemapped = 0xC8,
        ComPinsConfiguration = 0xDA,
        ContrastControl = 0x81,
        DeactivateScroll = 0x2E,
        DisplayOffset = 0xD3,
        DisplayOff = 0xAE,
        DisplayOn = 0xAF,
        DisplayStartLine = 0x40,
        EntireDisplayOn = 0xA5,
        EntireDisplayResume = 0xA4,
        HigherColumnStartAddress = 0x10,
        InverseDisplay = 0xA7,
        LeftHorizontalScroll = 0x27,
        LowerColumnStartAddress = 0x00,
        MemoryAddressingMode = 0x20,
        MultiplexRatio = 0xA8,
        Nop = 0xE3,
        NormalDisplay = 0xA6,
        OscillatorFrequency = 0xD5,
        PageAddress = 0x22,
        PageStartAddress = 0xB0,
        PreChargePeriod = 0xD9,
        RightHorizontalScroll = 0x26,
        SegmentReMap0 = 0xA0,
        SegmentReMap127 = 0xA1,
        VcomhDeselectLevel = 0xDB,
        VerticalLeftHorizontalScroll = 0x2A,
        VerticalRightHorizontalScroll = 0x29,
        VerticalScrollArea = 0xA3
    };

    inline
    void powerOn() {
        /* Power the display on. */

        /*
         * The sequence of commands that are sent to the chip when should_turn_power_on
         * gets set is stored with the driver, at the moment that sequence is:
         *   1. 40C8, VDD 0 (active low), wait 100*1ms
         *   2. 00AE, Display OFF, no wait
         *   3. 8002, Reset 0 (active low), wait 1*1ms
         *   4. 8003, Reset 1 (active low), wait 1*1ms
         *   5. 008D, Charge Pump Regulator:
         *   6. 0014, (Enable charge pump during display on)
         *   7. 00D9, PreChargePeriod:
         *   8. 00F1, (Phase 1 = 1 DCLK, Phase 2 = 15 DCLK)
         *   9. C0C8, VCC 0 (active low), wait 100*1ms
         *  10. 0081, ContrastControl:
         *  11. 000F, (15 from 0 - 255)
         *  12. 00A0, SegmentReMap0
         *  13. 00C0, ComOutputScanDirectionNormal
         *  14. 00DA, ComPinsConfiguration:
         *  15. 0000, (Sequential COM pins, Disable COM Left/Right remap)
         *  16. 0020, MemoryAddressingMode
         *  17. 0000, (Horizontal Addressing Mode)
         *  18. 32AF, Display ON, wait 50*4ms
         */

        SSD1306_DRIVER_mWriteReg(
            XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
            SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
            SHOULD_TURN_POWER_ON_MASK
        );
    }

    inline
    void powerOff() {
        /* Power the display off. */

        /*
         * The sequence of commands that are sent to the chip when should_turn_power_on
         * gets cleared is stored with the driver, at the moment that sequence is:
         *   1. 00AE; Display OFF, no wait
         *   2. C0C9; VCC 1 (active low), wait 100*1ms
         *   3. 4001; VDD 1 (active low), no wait
         */

        SSD1306_DRIVER_mWriteReg(
            XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
            SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
            ~SHOULD_TURN_POWER_ON_MASK & ~SHOULD_SEND_DIN_MASK
        );
    }

    inline
    void send(Command cmd) {
        /* Store original value of the control register (reg0). */
        u32 reg0 =
            SSD1306_DRIVER_mReadReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET
            );

        /* Sending commands makes sense only when the display is powered on. */
        if (reg0 & SHOULD_TURN_POWER_ON_MASK) {

            /* 8-bit transfer, value in reg1 is NOT data. */
            sendWithWait(
                (reg0 | IS_DIN_U8_MASK) & (~IS_DIN_DATA_MASK),
                cmd
            );

            /* Restore the original value of the control register reg0. */
            SSD1306_DRIVER_mWriteReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
                reg0
            );
        }
    }

    inline
    void send(Command cmd, u8 arg) {
        /* Store original value of the control register (reg0). */
        u32 reg0 =
            SSD1306_DRIVER_mReadReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET
            );

        /* Sending commands makes sense only when the display is powered on. */
        if (reg0 & SHOULD_TURN_POWER_ON_MASK) {

            /* 8-bit transfer, value in reg1 is NOT data. */
            u32 r0 = (reg0 | IS_DIN_U8_MASK) & (~IS_DIN_DATA_MASK);
            sendWithWait(r0, cmd);

            /* 8-bit transfer, value in reg1 is NOT data. */
            sendWithWait(r0, arg);

            /* Restore the original value of the control register reg0. */
            SSD1306_DRIVER_mWriteReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
                reg0
            );
        }
    }

    inline
    void send(Command cmd, u8 arg1, u8 arg2) {
        /* Store original value of the control register (reg0). */
        u32 reg0 =
            SSD1306_DRIVER_mReadReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET
            );

        /* Sending commands makes sense only when the display is powered on. */
        if (reg0 & SHOULD_TURN_POWER_ON_MASK) {

            /* 8-bit transfer, value in reg1 is NOT data. */
            u32 r0 = (reg0 | IS_DIN_U8_MASK) & (~IS_DIN_DATA_MASK);
            sendWithWait(r0, cmd);

            /* 8-bit transfer, value in reg1 is NOT data. */
            sendWithWait(r0, arg1);

            /* 8-bit transfer, value in reg1 is NOT data. */
            sendWithWait(r0, arg2);

            /* Restore the original value of the control register reg0. */
            SSD1306_DRIVER_mWriteReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
                reg0
            );
        }
    }

    inline
    void send(u32 data[], int nels) {
        /* Store original value of the control register (reg0). */
        u32 reg0 =
            SSD1306_DRIVER_mReadReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET
            );

        /* Sending data makes sense only when the display is powered on. */
        if (reg0 & SHOULD_TURN_POWER_ON_MASK) {

            for (int i = 0; i < nels; ++i) {

                /* 32-bit transfer, value in reg1 is data. */
                sendWithWait(
                    (reg0 | IS_DIN_DATA_MASK) & (~IS_DIN_U8_MASK),
                    data[i]
                 );
            }

            /* Restore the original value of the control register reg0. */
            SSD1306_DRIVER_mWriteReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
                reg0
            );
        }
    }

    inline
    void send(u8 data[], int nels) {
        /* Store original value of the control register (reg0). */
        u32 reg0 =
            SSD1306_DRIVER_mReadReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET
            );

        /* Sending data makes sense only when the display is powered on. */
        if (reg0 & SHOULD_TURN_POWER_ON_MASK) {

            for (int i = 0; i < nels; ++i) {

                /* 8-bit transfer, value in reg1 is data. */
                sendWithWait(
                    reg0 | IS_DIN_DATA_MASK | IS_DIN_U8_MASK,
                    data[i]
                 );
            }

            /* Restore the original value of the control register reg0. */
            SSD1306_DRIVER_mWriteReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
                reg0
            );
        }
    }

    inline void
    send(u8 din, bool is_din_data = false) {
        /* Store original value of the control register (reg0). */
        u32 reg0 =
            SSD1306_DRIVER_mReadReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET
            );

        /* Sending data makes sense only when the display is powered on. */
        if (reg0 & SHOULD_TURN_POWER_ON_MASK) {

            /* 8-bit transfer. */
            u32 r0 = reg0 | IS_DIN_U8_MASK;
            sendWithWait(
                is_din_data ? r0 | IS_DIN_DATA_MASK: r0 & ~IS_DIN_DATA_MASK,
                din
            );

            /* Restore the original value of the control register reg0. */
            SSD1306_DRIVER_mWriteReg(
                XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
                reg0
            );
        }
    }

private:
    /*
     * Wait in loop for indication that the driver is done with sending.
     */
    inline
    void sendWithWait(u32 reg0, u32 din) {

        /* Populate the register reg1 with value of din. */
        SSD1306_DRIVER_mWriteReg(
            XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
            SSD1306_DRIVER_S00_AXI_SLV_REG1_OFFSET,
            din
        );

        /* Signal the driver that din is ready to send. */
        SSD1306_DRIVER_mWriteReg(
            XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
            SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
            reg0 | SHOULD_SEND_DIN_MASK
        );

        waitForSendDone(reg0);
    }

    inline
    void waitForSendDone(u32 reg0) {
        u32 reg2;
        bool read_flag = false;
        do {
            reg2 =
                SSD1306_DRIVER_mReadReg(
                    XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                    SSD1306_DRIVER_S00_AXI_SLV_REG2_OFFSET
                );
            /* Clear the should_send_din flag after the first check of the is_busy status. */
            if (!read_flag) {
                reg0 &= ~SHOULD_SEND_DIN_MASK;
                SSD1306_DRIVER_mWriteReg(
                    XPAR_SSD1306_DRIVER_0_S00_AXI_BASEADDR,
                    SSD1306_DRIVER_S00_AXI_SLV_REG0_OFFSET,
                    reg0
                );
                read_flag = true;
            }
        } while (reg2 & IS_BUSY_MASK);
    }
};
