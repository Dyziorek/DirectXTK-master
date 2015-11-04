struct PSInputNoFog
{
	float4 Diffuse : COLOR0;
};

float4 PSBasicNoFog(PSInputNoFog pin) : SV_Target0
{
	return pin.Diffuse;
}