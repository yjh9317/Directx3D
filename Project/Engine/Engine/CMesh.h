#pragma once
#include "CRes.h"
#include "CFBXLoader.h"

class CStructuredBuffer;

struct tIndexInfo
{
    ComPtr<ID3D11Buffer>    pIB;
    D3D11_BUFFER_DESC       tIBDesc;
    UINT				    iIdxCount;
    void*                   pIdxSysMem;
};

class CMesh :
    public CRes
{
private:
    ComPtr<ID3D11Buffer>    m_VB;
    D3D11_BUFFER_DESC       m_tVBDesc;
    UINT                    m_iVtxCount;
    void*                   m_pVtxSys;

    // 하나의 버텍스버퍼에 여러개의 인덱스버퍼가 연결
    vector<tIndexInfo>		m_vecIdxInfo;

    // Animation3D 정보
    vector<tMTAnimClip>		m_vecAnimClip;
    vector<tMTBone>			m_vecBones;

    CStructuredBuffer*      m_pBoneFrameData;   // 전체 본 프레임 정보(크기, 이동, 회전) (프레임 개수만큼)
    CStructuredBuffer*      m_pBoneOffset;	    // 각 뼈의 offset 행렬(각 뼈의 위치를 되돌리는 행렬) (1행 짜리)
    

public:
    Vtx* GetVtxSysMem() { return (Vtx*)m_pVtxSys; }
    UINT GetSubsetCount() { return (UINT)m_vecIdxInfo.size(); }

    const vector<tIndexInfo>& GetvecIdxInfo() { return m_vecIdxInfo; }
    const vector<tMTBone>* GetBones() { return &m_vecBones; }
    UINT GetBoneCount() { return (UINT)m_vecBones.size(); }
    const vector<tMTAnimClip>* GetAnimClip() { return &m_vecAnimClip; }
    bool IsAnimMesh() { return !m_vecAnimClip.empty(); }

    CStructuredBuffer* GetBoneFrameDataBuffer() { return m_pBoneFrameData; } // 전체 본 프레임 정보
    CStructuredBuffer* GetBoneOffsetBuffer() { return  m_pBoneOffset; }	   // 각 뼈의 offset 행렬	

    UINT GetVtxCount() { return m_iVtxCount;}
public:
    virtual int Save(const wstring& _strFilePath);
    virtual int Load(const wstring& _strFilePath);
    
    int Create(void* _pVtxSys, UINT _iVtxCount, void* _pIdxSys, UINT _iIdxCount);
    static CMesh* CreateFromContainer(CFBXLoader& _loader);
    static CMesh* CreateFromContainer(CFBXLoader& _loader, int _idx);

    void render(UINT _iSubset);
    void render_particle(UINT _iCount);

    void UpdateData(UINT _iSubset);
public:
    CMesh();
    ~CMesh();
};

