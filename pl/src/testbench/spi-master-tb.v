`include "../verilog/timescale.vh"

module spi_master_tb;

    reg GCLK = 1;
    always
        #5 GCLK = ~GCLK;

    reg reset = 0;
    initial begin
        repeat(21) @(posedge GCLK, negedge GCLK);
        reset = 1;
    end

    reg is_data_ready = 0;
    reg is_data_width_8 = 1;
    reg [31:0] din = 0;
    wire sclk;
    wire cs;
    wire mo;
    spi_master #(
        .SCLK_DIVIDER(20)
    ) smi(
        .clk(GCLK),
        .is_data_ready(is_data_ready),
        .is_data_width_8(is_data_width_8),
        .data(din),
        .mosi(mo),
        .sclk(sclk),
        .cs(cs)
    );

    initial begin: main
        integer i;
        
        wait(reset);
        for (
            i = 3; 
            i < (is_data_width_8 ? 8'hFF : 32'h8FFFFFFF); 
            i = i + 13 * (1 << (is_data_width_8 ? 0 : 17))
        ) begin
            din = i;
            is_data_ready = 0;
            repeat (2) @(posedge GCLK);
            is_data_ready = 1;
            @(posedge GCLK);
            is_data_ready = 0;
            wait(cs);
            if (!is_data_width_8)
                repeat (3) begin
                    wait(!cs);
                    wait(cs); 
                end
        end

        $finish;
    end
endmodule
