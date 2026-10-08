#include "SpAbilityExtensionDefinition.h"
#include "ProjectSP/Common/SpLog.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

// ==================================================

FGameplayTag USpAbilityExtensionDefinition::FindAbilityKeyByExtension(const FGameplayTag ExtensionTag) const
{
	if (!ExtensionTag.IsValid())
		return FGameplayTag();

	if (!bExtensionIndexBuilt)
		RebuildExtensionIndex();

	const FGameplayTag* AbilityKey = ExtensionToAbility.Find(ExtensionTag);
	return AbilityKey ? *AbilityKey : FGameplayTag();
}

bool USpAbilityExtensionDefinition::FindOption(const FGameplayTag AbilityKey, const FGameplayTag ExtensionTag) const
{
	return AbilityKey.IsValid() && FindAbilityKeyByExtension(ExtensionTag) == AbilityKey;
}

const FSpAbilityExtensionGroup* USpAbilityExtensionDefinition::FindOptionByExtension(const FGameplayTag ExtensionTag) const
{
	const FGameplayTag AbilityKey = FindAbilityKeyByExtension(ExtensionTag);
	return AbilityKey.IsValid() ? Options.Find(AbilityKey) : nullptr;
}

// post

void USpAbilityExtensionDefinition::PostLoad()
{
	Super::PostLoad();
	RebuildExtensionIndex();
}

#if WITH_EDITOR

void USpAbilityExtensionDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildExtensionIndex();
}

void USpAbilityExtensionDefinition::PostEditUndo()
{
	Super::PostEditUndo();
	RebuildExtensionIndex();
}

EDataValidationResult USpAbilityExtensionDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	TMap<FGameplayTag, FGameplayTag> Index;
	TArray<FText> Errors;

	if (!BuildExtensionIndex(Index, Errors))
	{
		for (const FText& Error : Errors)
			Context.AddError(Error);

		return EDataValidationResult::Invalid;
	}

	return ParentResult == EDataValidationResult::Invalid ? ParentResult : EDataValidationResult::Valid;
}

#endif

// build dataset

bool USpAbilityExtensionDefinition::BuildExtensionIndex(TMap<FGameplayTag, FGameplayTag>& OutIndex, TArray<FText>& OutErrors) const
{
	OutIndex.Reset();
	OutErrors.Reset();

	for (const TPair<FGameplayTag, FSpAbilityExtensionGroup>& Pair : Options)
	{
		const FGameplayTag AbilityKey = Pair.Key;
		if (!AbilityKey.IsValid())
		{
			OutErrors.Add(FText::FromString(TEXT("Ability extension group has an invalid AbilityKey.")));
			continue;
		}

		for (const FGameplayTag& ExtensionTag : Pair.Value.Extensions)
		{
			if (!ExtensionTag.IsValid() || ExtensionTag == AbilityKey)
			{
				OutErrors.Add(FText::FromString(FString::Printf(TEXT("Invalid extension tag %s for ability %s."), *ExtensionTag.ToString(), *AbilityKey.ToString())));
				continue;
			}

			if (const FGameplayTag* ExistingAbilityKey = OutIndex.Find(ExtensionTag))
			{
				const FString Error = *ExistingAbilityKey == AbilityKey ? FString::Printf(TEXT("Extension tag %s is duplicated within ability %s."), *ExtensionTag.ToString(), *AbilityKey.ToString()) : FString::Printf(TEXT("Extension tag %s is duplicated in abilities %s and %s."), *ExtensionTag.ToString(), *ExistingAbilityKey->ToString(), *AbilityKey.ToString());
				OutErrors.Add(FText::FromString(Error));
				
				continue;
			}

			OutIndex.Add(ExtensionTag, AbilityKey);
		}
	}

	if (!OutErrors.IsEmpty())
		OutIndex.Reset();
	
	return OutErrors.IsEmpty();
}

void USpAbilityExtensionDefinition::RebuildExtensionIndex() const
{
	TMap<FGameplayTag, FGameplayTag> RebuiltIndex;
	TArray<FText> Errors;
	BuildExtensionIndex(RebuiltIndex, Errors);
	
	ExtensionToAbility = MoveTemp(RebuiltIndex);
	bExtensionIndexBuilt = true;

	for (const FText& Error : Errors)
	{
		UE_LOG(LogMj, Error, TEXT("%s: %s"), *GetName(), *Error.ToString());
	}
}
