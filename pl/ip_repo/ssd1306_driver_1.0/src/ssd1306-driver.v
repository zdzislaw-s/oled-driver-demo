`include "timescale.vh"

`define DATA_WIDTH 32

module ssd1306_driver #(
    SCLK_DIVIDER = 20
)(
    input clk,
    input should_turn_power_on,
    input should_send_din,
    input is_din_u8,
    input is_din_data,
    input [`DATA_WIDTH - 1:0] din,
    output is_busy,
    output ssd1306_vdd,
    output ssd1306_reset,
    output ssd1306_vcc,
    output ssd1306_dc,
    output ssd1306_cs,
    output ssd1306_sdin,
    output ssd1306_sclk
);
    localparam DATA_WIDTH = `DATA_WIDTH;

    reg oled_vdd = 1;
    assign ssd1306_vdd = oled_vdd;

    reg oled_reset = 1;
    assign ssd1306_reset = oled_reset;

    reg oled_vcc = 1;
    assign ssd1306_vcc = oled_vcc;

    reg oled_dc = 1;
    assign ssd1306_dc = oled_dc;

    /*
     * Instruction format:
     * 1      0       0
     * 5432109876543210
     * +------+-------+
     * 00wwwwwwdddddddd - send SSD1306 command [7:0], fetch next inst. after [13:8] 8ms
     * 01wwwwwwwwwwwwwd - set VDD on/off [0:0], fetch next inst. after [13:1] 1ms
     * 10wwwwwwwwwwwwwd - set RES# on/of [0:0], fetch next inst. after [13:1] 1ms
     * 11wwwwwwwwwwwwwd - set VCC/VBAT on/off, fetch next inst. after [13:1] 1ms
     *
     * Power ON instructions:
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
     *
     * Power OFF instructions:
     *   1. 00AE; Display OFF, no wait
     *   2. C0C9; VCC 1 (active low), wait 100*1ms
     *   3. 4001; VDD 1 (active low), no wait
     *
     * Notes:
     *  1. The commands are described in [1].
     *  2. The specification of the MCU is available in [2].
     *
     * [1] https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
     * [2] https://cdn-shop.adafruit.com/datasheets/UG-2832HSWEG04.pdf
     */
    localparam
        IP_POWER_ON_BASE = 0,
        IP_POWER_ON_MAX = 18;
    localparam
        IP_POWER_OFF_BASE = IP_POWER_ON_MAX,
        IP_POWER_OFF_MAX = IP_POWER_OFF_BASE + 3;
    localparam
        ROM_DEPTH = 256,
        ROM_WIDTH = 16;
    reg [$clog2(ROM_DEPTH) - 1:0] ip = 0; // the ROM is 256 words deep
    reg [$clog2(ROM_DEPTH) - 1:0] ip_max = 0;
    wire [ROM_WIDTH - 1:0] instruction; // word in ROM is 16 bits wide
    power_on_off_sequence power_on_off_sequence_inst(
        clk,
        ip,
        instruction
    );

    reg is_data_ready = 0;
    reg is_data_u8 = 0;
    reg [DATA_WIDTH - 1:0] data = { DATA_WIDTH {1'b0} };
    spi_master #(
        .SCLK_DIVIDER(SCLK_DIVIDER)
    ) spi_master_inst (
        .clk(clk),
        .is_data_ready(is_data_ready),
        .is_data_u8(is_data_u8),
        .data(is_data_ready ? data : { DATA_WIDTH {1'b0} }),
        .mosi(ssd1306_sdin),
        .sclk(ssd1306_sclk),
        .cs(ssd1306_cs),
        .is_busy(is_busy)
    );

    reg [(16 - 2 - 1) - 1:0] wait_time = 0;
    reg is_timer_enabled = 0;
    wire has_timer_timedout;
    timer #(
        //.CLK_CYCLES_NB(5) // TODO: SIM: switch this line off after simulating.
        .CLK_CYCLES_NB(100_000)
    ) timer_inst (
        .clk(clk),
        .timeout_time(wait_time), // time to timeout in ms
        .is_enabled(is_timer_enabled),
        .has_timedout(has_timer_timedout)
    );

    reg is_power_on = 0;
    reg is_executing_rom = 0;
    reg [1:0] send_sturtup_delay = 0;

    localparam
        Idle = 0,
        WaitDisplayOn = 1,
        WaitDisplayOff = 2,
        IncrementIp = 3,
        ExecuteInstruction = 4,
        WaitSendDone = 5,
        WaitTimeout = 6,
        StateMax = 7;
    reg [$clog2(StateMax) - 1:0] state = Idle;
    always @(posedge clk)
        case (state)

        Idle: begin
            // Turn the power on.
            if (should_turn_power_on && !is_power_on) begin
                ip <= IP_POWER_ON_BASE;
                ip_max <= IP_POWER_ON_MAX;
                is_executing_rom <= 1;
                state <= IncrementIp;
            end else
            // Turn the power off.
            if (!should_turn_power_on && is_power_on) begin
                ip <= IP_POWER_OFF_BASE;
                ip_max <= IP_POWER_OFF_MAX;
                is_executing_rom <= 1;
                state <= IncrementIp;
            end else
            // Send data that is provided by the caller.
            if (should_send_din && is_power_on && !is_busy) begin
                oled_dc <= is_din_data;
                data <= din;
                is_data_ready <= 1;

                // Commands are always 8bit wide.
                if (!is_din_data)
                    is_data_u8 <= 1;
                else
                    is_data_u8 <= is_din_u8;

                send_sturtup_delay <= 2;
                state <= WaitSendDone;
            end
        end

        IncrementIp: begin // to deal with 1cc latency of ROM
            if (ip == ip_max) begin
                is_executing_rom <= 0;
                is_power_on <= ip == IP_POWER_ON_MAX;
                state <= Idle;
            end else begin
                ip <= ip + 1;
                state <= ExecuteInstruction;
            end
        end

        ExecuteInstruction: begin
            case (instruction[15:14])
            2'b00: begin
                oled_dc <= 0;
                is_data_ready <= 1;
                is_data_u8 <= 1;
                data <= instruction[7:0];
                wait_time <= instruction[13:8] << 2;
                send_sturtup_delay <= 2;
                state <= WaitSendDone;
            end

            2'b01: begin
                oled_vdd <= instruction[0:0];
                wait_time <= instruction[13:1];
                state <= WaitTimeout;
            end

            2'b10: begin
                oled_reset <= instruction[0:0];
                wait_time <= instruction[13:1];
                state <= WaitTimeout;
            end

            2'b11: begin
                oled_vcc <= instruction[0:0];
                wait_time <= instruction[13:1];
                state <= WaitTimeout;
            end
            endcase
        end

        WaitSendDone: begin
            is_data_ready <= 0;
            // Allow spi-master to pick up is_data_ready and setup cs.
            send_sturtup_delay <= send_sturtup_delay - 1;
            if (send_sturtup_delay == 0 && !is_busy)
                if (wait_time > 0) begin
                    state <= WaitTimeout;
                end else begin
                    if (is_executing_rom)
                        state <= IncrementIp;
                    else
                        state <= Idle;
                end
        end

        WaitTimeout: begin
            if (wait_time > 0 && !is_timer_enabled)
                is_timer_enabled <= 1;
            else if (has_timer_timedout) begin
                is_timer_enabled <= 0;
                wait_time <= 0;
                state <= IncrementIp;
            end else if (wait_time == 0)
                state <= IncrementIp;
        end

        default:
            state <= Idle;

        endcase

endmodule
