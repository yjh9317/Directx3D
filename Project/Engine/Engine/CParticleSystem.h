#pragma once
#include "CRenderComponent.h"

#include "CStructuredBuffer.h"
#include "CParticleUpdateShader.h"

class CParticleSystem :
    public CRenderComponent
{
private:
    Ptr<CParticleUpdateShader>      m_CS;           // 파티클 업데이트 쉐이더

    CStructuredBuffer*              m_ParticleBuffer;   // 파티클 구조화 버퍼
    CStructuredBuffer*              m_DataBuffer;       // 추가 데이터 버퍼(파티클 AliveCount)
    UINT                            m_iMaxCount;        // 파티클 최대 개수

    int                             m_bPosInherit;      // 오브젝트 좌표에 영향
    int                             m_iAliveCount;      // 이번 프레임에 생성(활성화) 될 파티클 수
    
    float                           m_fMinLifeTime;     // 파티클의 최소 생명 시간
    float                           m_fMaxLifeTime;     // 파티클의 최대 생명 시간

    float                           m_fStartSpeed;      // 파티클의 최소 속력
    float                           m_fEndSpeed;        // 파티클의 최대 속력

    Vec4                            m_vStartColor;      // 파티클 초기 색상
    Vec4                            m_vEndColor;        // 파티클 최종 색상

    Vec3                            m_vStartScale;      // 파티클 초기 크기
    Vec3                            m_vEndScale;        // 파티클 최종 크기


    float                           m_fParticleCreateDistance;  // 파티클 생성 범위
    float                           m_fParticleCreateTerm;      // 파티클 생성 간격
    float                           m_fAccTime;
    
    int                             m_iEmissive;        // 파티클 발광여부
    int                             m_iLighting;        // 광원 적용 여부

public:
    void SetEmissive(bool _bEmissive);
    void SetApplyLighting(bool _bLighting);
    void SetMaxParticleCount(UINT _iMax);

    bool GetEmissive() { return m_iEmissive; }
    bool GetApplyLighting() { return m_iLighting; }

    int GetAliveCount() { return m_iAliveCount; }
    void SetAliveCount(int _i) { m_iAliveCount = _i; }

    float GetMaxLifeTime() { return m_fMaxLifeTime; }
    void SetMaxLifeTime(float _f) { m_fMaxLifeTime = _f; }

    float GetMinLifeTime() { return m_fMinLifeTime; }
    void SetMinLifeTime(float _f) { m_fMinLifeTime = _f; }

    float GetStartSpeed() { return m_fStartSpeed; }
    void SetStartSpeed(float _f) { m_fStartSpeed = _f; }

    float GetEndSpeed() { return m_fEndSpeed; }
    void SetEndSpeed(float _f) { m_fEndSpeed = _f; }

    Vec4 GetStartColor() { return m_vStartColor; }
    void SetStartColor(Vec4 _v) { m_vStartColor = _v; }

    Vec4 GetEndColor() { return m_vEndColor; }
    void SetEndColor(Vec4 _v) { m_vEndColor = _v; }

    Vec3 GetStartScale() { return m_vStartScale; }
    void SetStartScale(Vec3 _v) { m_vStartScale = _v; }

    Vec3 GetEndScale() { return m_vEndScale; }
    void SetEndScale(Vec3 _v) { m_vEndScale = _v; }

    float GetCreateDist() { return m_fParticleCreateDistance; }
    void SetCreateDist(float _f) { m_fParticleCreateDistance = _f; }

    float GetCreateTerm() { return m_fParticleCreateTerm; }
    void SetCreateTerm(float _f) { m_fParticleCreateTerm = _f; }
    

public:
    virtual void finalupdate() override;
    virtual void render()   override;


public:
    virtual void SaveToScene(FILE* _pFile) override;
    virtual void LoadFromScene(FILE* _pFile) override;

    virtual void SaveJsonFile(Json::Value& root) override;
    virtual void LoadJsonFromFile(Json::Value& root) override;
    CLONE(CParticleSystem);
public:
    CParticleSystem();
    CParticleSystem(const CParticleSystem& _origin);
    ~CParticleSystem();
};

