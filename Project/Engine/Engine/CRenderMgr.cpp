#include "pch.h"
#include "CRenderMgr.h"

#include "CTimeMgr.h"

#include "CDevice.h"
#include "CConstBuffer.h"

#include "CTransform.h"
#include "CCamera.h"
#include "CResMgr.h"
#include "CLight2D.h"
#include "CLight3D.h"
#include "CCollider3D.h"
#include "CEventMgr.h"

#include "CStructuredBuffer.h"
#include "CSceneMgr.h"
#include "CScene.h"
#include "CLayer.h"

#include "CMRT.h"

CRenderMgr::CRenderMgr()
	: m_pEditorCam(nullptr)
	, m_pUICamera(nullptr)
	, m_pLight2DBuffer(nullptr)
	, m_pLight3DBuffer(nullptr)
	, m_arrMRT{}
	, m_pMergeShader(nullptr)
	, m_pMergeMtrl(nullptr)
{
	m_pLight2DBuffer = new CStructuredBuffer;
	m_pLight2DBuffer->Create(sizeof(tLightInfo), 2, SB_TYPE::READ_ONLY, true, nullptr);

	m_pLight3DBuffer = new CStructuredBuffer;
	m_pLight3DBuffer->Create(sizeof(tLightInfo), 2, SB_TYPE::READ_ONLY, true, nullptr);
}

CRenderMgr::~CRenderMgr()
{
	SAFE_DELETE(m_pLight2DBuffer);
	SAFE_DELETE(m_pLight3DBuffer);

	Safe_Del_Arr(m_arrMRT);

	SAFE_DELETE(m_pMergeShader);
	SAFE_DELETE(m_pMergeMtrl);
}

void CRenderMgr::update()
{

}

void CRenderMgr::render()
{
	// Rendering 시작	
	ClearMRT();	

	// TextureRegister 초기화
	ClearTextureRegister();

	// Light 업데이트
	UpdateLight2D();
	UpdateLight3D();

	// Global 상수 업데이트
	static CConstBuffer* pGlobalCB = CDevice::GetInst()->GetCB(CB_TYPE::GLOBAL);
	pGlobalCB->SetData(&g_global, sizeof(tGlobal));
	pGlobalCB->UpdateData();
	pGlobalCB->UpdateData_CS();


	CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();

	if (pCurScene->GetSceneState() == SCENE_STATE::PLAY)
	{
		render_play();
	}

	else
	{
		render_editor();
	}	

	m_vecLight2D.clear();
	m_vecLight3D.clear();
}

void CRenderMgr::render_play()
{
	if (m_vecCam.empty())
		return;

	GetUICam();

	CCamera* pMainCam = m_vecCam[0];

	Ptr<CMaterial> pMtrl = CResMgr::GetInst()->FindRes<CMaterial>(L"material\\UIMtrl.mtrl");
	Matrix tmp = m_vecCam[0]->m_Orthographic;
	pMainCam->GetViewMat()._11;
	pMainCam->GetViewMat()._13;
	pMainCam->GetViewMat()._31;
	pMainCam->GetViewMat()._33;

	Matrix b = {};
	b._11 = pMainCam->GetViewMat()._11;
	b._13 = pMainCam->GetViewMat()._13;
	b._31 = pMainCam->GetViewMat()._31;
	b._33 = pMainCam->GetViewMat()._33;
	Matrix c = XMMatrixInverse(nullptr, b);

	pMtrl->SetScalarParam(SCALAR_PARAM::MAT_0, &tmp);
	pMtrl->SetScalarParam(SCALAR_PARAM::MAT_1, &c);

	pMtrl = CResMgr::GetInst()->FindRes<CMaterial>(L"material\\BackgroundHPUI.mtrl");
	pMtrl->SetScalarParam(SCALAR_PARAM::MAT_0, &tmp);
	pMtrl->SetScalarParam(SCALAR_PARAM::MAT_1, &c);


	// Camera 가 찍는 Layer 의 오브젝트들을 Shader Domain 에 따라 분류해둠
	pMainCam->SortGameObject();

	render_shadowmap();

	g_transform.matView = pMainCam->GetViewMat();
	g_transform.matViewInv = pMainCam->GetViewInvMat();
	g_transform.matProj = pMainCam->GetProjMat();

	// Deferred 물체 렌더링			
	m_arrMRT[(UINT)MRT_TYPE::DEFERRED]->OMSet();
	pMainCam->render_deferred();

	m_arrMRT[(UINT)MRT_TYPE::DEFERRED_DECAL]->OMSet();
	pMainCam->render_deferred_decal();


	// 광원 렌더링
	render_lights();

	// Merge
	m_arrMRT[(UINT)MRT_TYPE::SWAPCHAIN]->OMSet();

	Ptr<CMesh> pRectMesh = CResMgr::GetInst()->FindRes<CMesh>(L"RectMesh");
	int a = 0;
	m_pMergeMtrl->SetScalarParam(SCALAR_PARAM::INT_0, &a);
	m_pMergeMtrl->UpdateData();
	pRectMesh->render(0);


	// Foward 물체 렌더링	
		
	pMainCam->render_forward();

	// Masked 물체 렌더링
	pMainCam->render_masked();

	// Foward Decal 렌더링
	pMainCam->render_forward_decal();

	// Alpha 물체 렌더링
	pMainCam->render_translucent();

	render_collider();

	// PostProcess 물체 렌더링
	pMainCam->render_postprocess();

	pMainCam->render_ui();

	// Sub 카메라 시점으로 렌더링
	for (int i = 1; i < m_vecCam.size(); ++i)
	{
		if (nullptr == m_vecCam[i])
			continue;

		m_vecCam[i]->SortGameObject();

		g_transform.matView = m_vecCam[i]->GetViewMat();
		g_transform.matProj = m_vecCam[i]->GetProjMat();

		// Foward 물체 렌더링
		m_vecCam[i]->render_forward();

		// Masked 물체 렌더링
		m_vecCam[i]->render_masked();

		// Alpha 물체 렌더링
		m_vecCam[i]->render_translucent();
	}

}

void CRenderMgr::render_editor()
{
	if (nullptr == m_pEditorCam)
		return;


	Ptr<CMaterial> pMtrl = CResMgr::GetInst()->FindRes<CMaterial>(L"material\\UIMtrl.mtrl");
	Matrix tmp = m_pEditorCam->m_Orthographic;
	m_pEditorCam->GetViewMat()._11;
	m_pEditorCam->GetViewMat()._13;
	m_pEditorCam->GetViewMat()._31;
	m_pEditorCam->GetViewMat()._33;

	Matrix b = {};
	b._11 = m_pEditorCam->GetViewMat()._11;
	b._13 = m_pEditorCam->GetViewMat()._13;
	b._31 = m_pEditorCam->GetViewMat()._31;
	b._33 = m_pEditorCam->GetViewMat()._33;
	Matrix c = XMMatrixInverse(nullptr, b);

	pMtrl->SetScalarParam(SCALAR_PARAM::MAT_0, &tmp);
	pMtrl->SetScalarParam(SCALAR_PARAM::MAT_1, &c);

	//pMtrl = CResMgr::GetInst()->FindRes<CMaterial>(L"material\\BackgroundHPUI.mtrl");
	//pMtrl->SetScalarParam(SCALAR_PARAM::MAT_0, &tmp);
	//pMtrl->SetScalarParam(SCALAR_PARAM::MAT_1, &c);


	// 에디터 카메라 시점으로 렌더링
	// Camera 가 찍는 Layer 의 오브젝트들을 Shader Domain 에 따라 분류해둠
	m_pEditorCam->SortGameObject();

	// Directional Light ShadowMap 만들기
	render_shadowmap();


	g_transform.matView = m_pEditorCam->GetViewMat();
	g_transform.matViewInv = m_pEditorCam->GetViewInvMat();
	g_transform.matProj = m_pEditorCam->GetProjMat();

	// Deferred 물체 렌더링			
	m_arrMRT[(UINT)MRT_TYPE::DEFERRED]->OMSet();
	m_pEditorCam->render_deferred();

	m_arrMRT[(UINT)MRT_TYPE::DEFERRED_DECAL]->OMSet();
	m_pEditorCam->render_deferred_decal();

	// 광원 렌더링
	render_lights();

	// Merge
	m_arrMRT[(UINT)MRT_TYPE::SWAPCHAIN]->OMSet();

	Ptr<CMesh> pRectMesh = CResMgr::GetInst()->FindRes<CMesh>(L"RectMesh");
	int a = 0;
	m_pMergeMtrl->SetScalarParam(SCALAR_PARAM::INT_0, &a);
	m_pMergeMtrl->UpdateData();
	pRectMesh->render(0);

	// Foward 물체 렌더링	
	m_pEditorCam->render_forward();

	// Masked 물체 렌더링
	m_pEditorCam->render_masked();

	// Foward Decal 렌더링
	m_pEditorCam->render_forward_decal();

	// Alpha 물체 렌더링
	m_pEditorCam->render_translucent();

	render_collider();
	
	// PostProcess 물체 렌더링
	m_pEditorCam->render_postprocess();	

	m_pEditorCam->render_ui();
}

void CRenderMgr::render_shadowmap()
{
	m_arrMRT[(UINT)MRT_TYPE::SHADOWMAP]->OMSet();

	for (size_t i = 0; i < m_vecLight3D.size(); ++i)
	{
		if(LIGHT_TYPE::DIRECTIONAL == m_vecLight3D[i]->GetLightType())
			m_vecLight3D[i]->render_shadowmap();
	}
}

void CRenderMgr::render_lights()
{
	m_arrMRT[(UINT)MRT_TYPE::LIGHT]->OMSet();

	for (size_t i = 0; i < m_vecLight3D.size(); ++i)
	{
		m_vecLight3D[i]->render();
	}
}

void CRenderMgr::render_collider()
{
	CScene* pCurScene = CSceneMgr::GetInst()->GetCurScene();
	for (int i = 0; i < 32; ++i)
	{
		CLayer* pLayer = pCurScene->GetLayer(i);

		vector<CGameObject*> vObj = pLayer->GetObjects();
		
		for (int j = 0; j < vObj.size(); ++j)
		{
			CCollider3D* pCol = vObj[j]->Collider3D();

			if (nullptr != pCol)
				pCol->render();
		}
	}
}


void CRenderMgr::RegisterCamera(CCamera* _pCam)
{
	// 카메라가 RenderMgr에 최초 등록 되는 경우
	if (-1 == _pCam->m_iCamIdx)
	{
		m_vecCam.push_back(_pCam);
		int iIdx = (int)m_vecCam.size() - 1;
		_pCam->m_iCamIdx = iIdx;
	}	
	else
	{
		if (m_vecCam.size() <= _pCam->m_iCamIdx)
		{
			m_vecCam.resize((size_t)_pCam->m_iCamIdx + 1);
		}

		m_vecCam[_pCam->m_iCamIdx] = _pCam;
	}
}

void CRenderMgr::SwapCameraIndex(CCamera* _pCam, int _iChangeIdx)
{
	for (size_t i = 0; i < m_vecCam.size(); ++i)
	{
		if (_pCam == m_vecCam[i])
		{
			if (nullptr != m_vecCam[_iChangeIdx])
			{
				m_vecCam[_iChangeIdx]->m_iCamIdx = (int)i;
				_pCam->m_iCamIdx = _iChangeIdx;

				return;
			}
		}
	}

	assert(nullptr);
}

void CRenderMgr::CopyTargetToPostProcess()
{
	Ptr<CTexture> pRenderTarget = CResMgr::GetInst()->FindRes<CTexture>(L"RenderTargetTex");
	Ptr<CTexture> pPostProcess = CResMgr::GetInst()->FindRes<CTexture>(L"PostProcessTex");

	CONTEXT->CopyResource(pPostProcess->GetTex2D().Get(), pRenderTarget->GetTex2D().Get());
}

void CRenderMgr::UpdateLight2D()
{
	if (m_pLight2DBuffer->GetElementCount() < m_vecLight2D.size())
	{
		m_pLight2DBuffer->Create((UINT)sizeof(tLightInfo), (UINT)m_vecLight2D.size(), SB_TYPE::READ_ONLY, true, nullptr);
	}

	static vector<tLightInfo> vecLight2DInfo;
	vecLight2DInfo.clear();

	for (size_t i = 0; i < m_vecLight2D.size(); ++i)
	{
		vecLight2DInfo.push_back(m_vecLight2D[i]->GetLightInfo());
	}
	m_pLight2DBuffer->SetData(vecLight2DInfo.data(), (UINT)vecLight2DInfo.size());
	m_pLight2DBuffer->UpdateData(PIPELINE_STAGE::PS, 60);

	g_global.Light2DCount = (int)m_vecLight2D.size();

	
}

void CRenderMgr::UpdateLight3D()
{
	if (m_pLight3DBuffer->GetElementCount() < m_vecLight3D.size())
	{
		m_pLight3DBuffer->Create((UINT)sizeof(tLightInfo), (UINT)m_vecLight3D.size(), SB_TYPE::READ_ONLY, true, nullptr);
	}

	static vector<tLightInfo> vecLight3DInfo;
	vecLight3DInfo.clear();

	for (size_t i = 0; i < m_vecLight3D.size(); ++i)
	{
		vecLight3DInfo.push_back(m_vecLight3D[i]->GetLightInfo());
	}
	m_pLight3DBuffer->SetData(vecLight3DInfo.data(), (UINT)vecLight3DInfo.size());
	m_pLight3DBuffer->UpdateData(PIPELINE_STAGE::PS, 61);

	g_global.Light3DCount = (int)m_vecLight3D.size();

	
}

// 현재 시점 카메라 가져오기
CCamera* CRenderMgr::GetMainCam() const
{
	if (SCENE_STATE::PLAY == CSceneMgr::GetInst()->GetCurScene()->GetSceneState())
	{
		if (m_vecCam.empty())
			return nullptr;

		return m_vecCam[0];
	}
	else
	{
		return m_pEditorCam;
	}
}

CCamera* CRenderMgr::GetUICam()
{
	if (nullptr != m_pUICamera)
		return m_pUICamera;

	if (SCENE_STATE::PLAY != CSceneMgr::GetInst()->GetCurScene()->GetSceneState())
		return m_pEditorCam;
	

	for (int i = 0; i < m_vecCam.size(); ++i)
	{
		if (m_vecCam[i]->GetName() == "UICamera")
		{
			m_pUICamera = m_vecCam[i];
			return m_pUICamera;
		}
	}

	return nullptr;
}
