`include "timescale.vh"

// maximum timeout 1s = 1000*1ms
module timer #(
    parameter CLK_CYCLES_NB = 100_000
) (
    input clk,
    input [12:0] timeout_time, // time to timeout in ms
    input is_enabled, // start timer
    output reg has_timedout // signal timeout
);
    localparam 
        COUNTER_WIDTH = $clog2(1_000 * CLK_CYCLES_NB); // 1ms = 1,000us = (@100Mz) 100,000 cc

    reg [COUNTER_WIDTH - 1:0] couter_max1;
    reg [COUNTER_WIDTH - 1:0] couter_max;
    always @(posedge clk) begin
        couter_max <= CLK_CYCLES_NB * timeout_time; 
        // To address [DRC DPOP-1] we would have extra register stage, but
        // this has it's own proglems so no, no extra stages.
    end

    reg [COUNTER_WIDTH - 1:0] counter;
    always @(posedge clk)
        if (!is_enabled) begin
            counter <= 0;
            has_timedout <= 0;
        end else begin
            if (counter == couter_max)
                has_timedout <= 1;
            counter <= counter + 1; // .01us elapsed
        end
endmodule
