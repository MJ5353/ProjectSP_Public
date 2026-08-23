#include "SPAssetManager.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"

// ==================================================

void USPAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	FSpGameplayTags::InitializeNativeTags();
}

USPAssetManager& USPAssetManager::Get()
{
	check(GEngine);
	
	USPAssetManager* Singleton = Cast<USPAssetManager>(GEngine->AssetManager);
	check(Singleton);
	
	return *Singleton;
}

UObject* USPAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	// 필요한 것만 동기로 로드하기. 
	// FSoftObjectPath : 로드할 오브젝트의 경로만 들고 있음
	
	if (!AssetPath.IsValid())
		return nullptr;
	
	// AssetManager가 있으면, AssetManager의 StreamableManager를 통해 정적 로딩
	// GetStreamableManager() : 로딩을 관리하는 클래스
	if (IsInitialized())
		return GetStreamableManager().LoadSynchronous(AssetPath);

	// asset manager 미초기화된 경우, FSoftObjectPath를 통해 바로 정적 로딩
	// 웬만하면 TryLoad는 안 써야 한다. (very slow)
	return AssetPath.TryLoad();
}

void USPAssetManager::AddLoadedAsset(const UObject* Asset)
{
	// ensure: 처음 실패했을 때만 로그/콜스택/디버거 브레이크
	// ensureAlways: 그 이후 같은 위치에서 또 실패해도 보통 조용히 false만 반환
	
	if (ensureAlways(Asset) == false)
		return;

	// FScopeLock은 생성자에서 SyncObject.Lock()을 호출하고, 소멸자에서 자동으로 SyncObject.Unlock()을 호출하는 객체
	FScopeLock Lock(&SyncObject);
	LoadedAssets.Add(Asset);
}
