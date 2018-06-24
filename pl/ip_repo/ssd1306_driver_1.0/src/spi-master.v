`include "timescale.vh"

`define WIDTH_32 32

module spi_master #(
    parameter SCLK_DIVIDER = 20
) (
    input clk, // reference clock
    input is_data_ready, // signal whether data is ready for sending
    input is_data_u8, // data can be passed in as either 8 or 32 bit entity
    input [`WIDTH_32 - 1:0] data, // data to be sent
    output mosi, // output bit
    output sclk, // SPI clock
    output reg cs, // mosi is read of when cs is low
    output is_busy // are we still putting bits on mosi?
);
    localparam WIDTH_32 = `WIDTH_32;
    localparam WIDTH_8 = 8;

    wire is_sending; 
    reg is_sclk_falling = 0;
    reg is_sclk_rising = 0;
    reg [1:0] byte_cnt = 0;
    
    reg [WIDTH_32 - 1:0] dout = 8'h00;
    always @(posedge clk)
        if (is_data_ready && !is_sending && byte_cnt == 0)
            dout <= data;
        else
            if (is_sending && is_sclk_falling)
                dout <= { dout[WIDTH_32 - 2:0], 1'b0 };
                    
    assign mosi = is_data_u8 ? dout[WIDTH_8 - 1] : dout[WIDTH_32 - 1];
    
    always @(posedge clk)
        if (is_data_ready || (!is_data_u8 && byte_cnt > 0 && cs == 1))
            cs <= 0;
        else
            if (is_sending)
                cs <= 0;
            else
                cs <= 1;
    
    reg [$clog2(WIDTH_8):0] counter = 0;
    always @(posedge clk)
        if (is_sending) begin
            if (is_sclk_rising) begin
                // keep ticking (<=) to deliver the LSB
                if (counter <= WIDTH_8 + 1)
                    counter <= counter + 1;
                else
                    counter <= 0;
            end
        end else
            counter <= 0;
    
    always @(posedge clk)
        if (counter == WIDTH_8 - 1 && is_sclk_rising)
            if (is_data_u8)
                byte_cnt <= 0;
            else if (!is_data_u8 && byte_cnt == 3)
                byte_cnt <= 0;
            else
                byte_cnt <= byte_cnt + 1;

    reg [$clog2(SCLK_DIVIDER):0] sclk_counter = 0;
    always @(posedge clk)
        if (is_sending)
            if (sclk_counter < SCLK_DIVIDER)
                sclk_counter <= sclk_counter + 1;
            else
                sclk_counter <= 0;
        else
            sclk_counter <= 0;
            
    localparam 
        SCLK_DIVIDERb2 = SCLK_DIVIDER / 2,
        SCLK_DIVIDERl1 = SCLK_DIVIDER - 1;
    always @(posedge clk) 
        if (is_sending) begin
            if (sclk_counter == SCLK_DIVIDERb2)
                is_sclk_rising <= 1;
            else
                is_sclk_rising <= 0;
            
            if (sclk_counter == SCLK_DIVIDERl1)
                is_sclk_falling <= 1;
            else
                is_sclk_falling <= 0;
        end
    
    assign is_sending = (
        (cs == 0 && counter == 0) || 
        (counter > 0 && counter < WIDTH_8) ||
        (counter == WIDTH_8 && sclk_counter > SCLK_DIVIDERb2)
    );
    assign sclk = is_sending ? sclk_counter >= SCLK_DIVIDERb2 : 1'b1;
    assign is_busy = (byte_cnt > 0 || !cs);
    
endmodule
