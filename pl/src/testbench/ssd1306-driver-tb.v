`include "../verilog/timescale.vh"

module ssd1306_driver_tb;

    reg GCLK = 1;
    always
        #5 GCLK = ~GCLK;

    reg reset = 1;
    initial begin
        repeat(8) @(posedge GCLK);
        reset = 0;
        repeat(8) @(posedge GCLK);
        reset = 1;
    end

    wire ssd1306_vdd;
    wire ssd1306_vcc;
    wire ssd1306_reset;
    wire ssd1306_cs;
    wire ssd1306_dc;
    wire ssd1306_sdin;
    wire ssd1306_sclk;
    reg should_turn_power_on;
    reg should_send_din;
    reg [`DATA_WIDTH - 1:0] din = 0;
    reg is_din_data = 0;
    ssd1306_driver uut (
        .clk(GCLK),
        .should_turn_power_on(should_turn_power_on),
        .should_send_din(should_send_din),
        .is_din_data(is_din_data),
        .din(din),
        .ssd1306_vdd(ssd1306_vdd),
        .ssd1306_reset(ssd1306_reset),
        .ssd1306_vcc(ssd1306_vcc),
        .ssd1306_cs(ssd1306_cs),
        .ssd1306_dc(ssd1306_dc),
        .ssd1306_sdin(ssd1306_sdin),
        .ssd1306_sclk(ssd1306_sclk)
    );

    initial begin
        wait(reset);
        wait(!reset);

        should_turn_power_on = 0;
        #(1000); //1us

        should_turn_power_on = 1;
        wait(!ssd1306_vcc);
        repeat(1) @(posedge ssd1306_cs);
        #(100) //100ns

        should_send_din = 1;
        is_din_data = 1'b0;
        din = `DATA_WIDTH'hA5;
        repeat(1) @(posedge ssd1306_cs);
        should_send_din = 0;

        #(100) //100ns

        should_turn_power_on = 0;
        wait(ssd1306_vdd);
        #(10*100);

        $finish;
    end

endmodule
