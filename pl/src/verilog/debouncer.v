`include "timescale.vh"

module debouncer #(
    parameter WIDTH = 1,
    parameter BOUNCE_LIMIT = 1024
) (
    input clk,
    input [WIDTH - 1:0] switch_in,
    output reg [WIDTH - 1:0] switch_out,
    output reg [WIDTH - 1:0] switch_rise,
    output reg [WIDTH - 1:0] switch_fall
);

    genvar i;
    generate
        for (i = 0; i < WIDTH; i = i + 1) begin
            reg [$clog2(BOUNCE_LIMIT) - 1:0] bounce_count = 0;

            reg [1:0] switch_shift = 0;
            always @(posedge clk)
                switch_shift <= { switch_shift, switch_in[i] };

            always @(posedge clk)
                if (bounce_count == 0) begin
                    switch_rise[i] <= switch_shift == 2'b01;
                    switch_fall[i] <= switch_shift == 2'b10;
                    switch_out[i] <= switch_shift[0];
                    if (switch_shift[1] != switch_shift[0])
                        bounce_count <= BOUNCE_LIMIT - 1;
                end else begin
                    switch_rise[i] <= 0;
                    switch_fall[i] <= 0;
                    bounce_count <= bounce_count - 1;
                end
        end
    endgenerate

endmodule
