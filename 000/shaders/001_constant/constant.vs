
// 頂点シェーダ
// Debug:   fxc /E vs_main /T vs_5_0 /Fh src/constant.vs.h /Od /Vn g_vs_constant /Zi ../000/shaders/001_constant/constant.vs
// Release: fxc /E vs_main /T vs_5_0 /Fh src/constant.vs.h /O1 /Vn g_vs_constant ../000/shaders/001_constant/constant.vs

struct VS_IN
{
	float3 position : IN_POSITION;
	float4 color    : IN_COLOR;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float4 color    : COLOR0;
};


VS_OUT vs_main( VS_IN VSin )
{
	VS_OUT VSout;
	VSout.position = float4( VSin.position, 1.0f );
	VSout.color    = VSin.color;

	return VSout;
}

