#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "SPAssetManager.generated.h"

// ==================================================

/**
 * 커스텀 AssetManager
 */
UCLASS()
class PROJECTSP_API USPAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TSet<TObjectPtr<const UObject>> LoadedAssets;
	
	// Object 단위 Lock용
	FCriticalSection SyncObject;

	// ------------------------------------------------
	
	static USPAssetManager& Get();
	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);
	
	// thread safe - 로딩된 에셋 캐싱하기
	void AddLoadedAsset(const UObject* Asset);
	
	// template
	template <typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template <typename AssetType>
	static TSubclassOf<AssetType> GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);
};

// template 함수 구현 ==================================================

template <typename AssetType>
AssetType* USPAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;
	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
			
			// Unreal의 비치명적 검증 매크로, false이면 로그만
			ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
		}

		if (LoadedAsset && bKeepInMemory)
			Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
	}

	return LoadedAsset;
}

template <typename AssetType>
TSubclassOf<AssetType> USPAssetManager::GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();
	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
			
			// Unreal의 비치명적 검증 매크로, false이면 로그만
			ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
		}

		if (LoadedSubclass && bKeepInMemory)
			Get().AddLoadedAsset(Cast<UObject>(LoadedSubclass));
	}

	return LoadedSubclass;
}
