#define scale 0.015873016

cbuffer rasterData : register (b0)
{
	row_major matrix<float, 4, 4>  u_matrix;
	row_major matrix<float, 4, 4>  u_exmatrix;
	float u_ratio;
	float2 u_linewidth;
	float4 u_color;
	float u_point;
	float u_blur;
	float2 u_pattern_size_a;
	float2 u_pattern_tl_a;
	float2 u_pattern_br_a;
	float2 u_pattern_size_b;
	float2 u_pattern_tl_b;
	float2 u_pattern_br_b;
	float u_fade;
	float u_opacity;
};

struct PixelShaderInput
{
	float4 pos : SV_Position;
	float4 patternAB :POSITION0;
};

PixelShaderInput BasicVS(float4 a_pos : POSITION, float4 a_data : NORMAL)
{
	PixelShaderInput psIN;

	float2 a_extrude = a_data.xy;

	float2 normal = fmod(a_pos.xy, 2.0);
	normal.y = sign(normal.y - 0.5);
	psIN.patternAB = float4(normal, 0, 0);
	float4 dist = float4(mul(mul(u_linewidth.x, a_extrude), scale), 0.0, 0.0);
	psIN.pos = mul(u_matrix, float4(floor(a_pos.xy * 0.5), 0.0, 1.0)) + mul(u_exmatrix, dist);
	return psIN;
}


