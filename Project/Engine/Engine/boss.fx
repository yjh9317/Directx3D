#ifndef _BOSS
#define _BOSS

#include "value.fx"

struct VTX_IN
{
    float3 vPos : POSITION;
    float2 vUV : TEXCOORD;

    float3 vTangent : TANGENT;
    float3 vNormal : NORMAL;
    float3 vBinormal : BINORMAL;

    float4 vWeights : BLENDWEIGHT;
    float4 vIndices : BLENDINDICES;
};

struct VTX_OUT
{
    float4 vPosition : SV_Position;
    float2 vUV : TEXCOORD;

    float3 vViewPos : POSITION;
    float3 vViewTangent : TANGENT;
    float3 vViewNormal : NORMAL;
    float3 vViewBinormal : BINORMAL;
};

// =========================
// Std3D_Deferred
// g_tex_0 : Output Texture
// g_tex_1 : Normal Map
// DOMAIN : Deferred
// Rasterizer : CULL_BACK
// DepthStencilState : LESS
// BlendState : DEFAULT
// =========================
VTX_OUT VS_Boss(VTX_IN _in)
{
    VTX_OUT output = (VTX_OUT)0.f;

    if (g_iAnim)
    {
        Skinning(_in.vPos, _in.vTangent, _in.vBinormal, _in.vNormal, _in.vWeights, _in.vIndices, 0);
    }

    output.vPosition = mul(float4(_in.vPos, 1.f), g_matWVP);

    output.vViewPos = mul(float4(_in.vPos, 1.f), g_matWV).xyz;
    output.vViewTangent = normalize(mul(float4(_in.vTangent, 0.f), g_matWV)).xyz;
    output.vViewNormal = normalize(mul(float4(_in.vNormal, 0.f), g_matWV)).xyz;
    output.vViewBinormal = normalize(mul(float4(_in.vBinormal, 0.f), g_matWV)).xyz;
    output.vUV = _in.vUV;

    return output;
}

struct PS_OUT
{
    float4 vColor       : SV_Target0;
    float4 vNormal      : SV_Target1;
    float4 vPosition    : SV_Target2;
    float4 vData        : SV_Target3;
};

PS_OUT PS_Boss(VTX_OUT _in)
{
    PS_OUT output = (PS_OUT)0.f;

    output.vColor = float4(1.f, 0.f, 1.f, 1.f);

    if (g_btex_0)
    {
        output.vColor = g_tex_0.Sample(g_sam_0, _in.vUV);
    }


    // 노말맵핑
    float3 vViewNormal = _in.vViewNormal;
    if (g_btex_1)
    {
        float3 vNormal = g_tex_1.Sample(g_sam_0, _in.vUV).rgb;
        vNormal = vNormal * 2.f - 1.f; // 0~1 값을 -1 ~ 1 로 확장       

        float3x3 matRot =
        {
            _in.vViewTangent
            , _in.vViewBinormal
            , _in.vViewNormal
        };

        vViewNormal = normalize(mul(vNormal, matRot));
    }

    output.vNormal = float4(vViewNormal, 1.f);
    output.vPosition = float4(_in.vViewPos, 1.f);
    output.vData = float4(0.f, 0.f, 0.f, 1.f);

    return output;
}


#endif