#include "SpAbilityTask_WaitReplicatedGameplayEvent.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"

// ==================================================

USpAbilitySystemComponent* USpAbilityTask_WaitReplicatedGameplayEvent::GetSpAbilitySystemComponent() const
{
	return Cast<USpAbilitySystemComponent>(AbilitySystemComponent.Get());
}

void USpAbilityTask_WaitReplicatedGameplayEvent::Activate()
{
	USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	if (!ASC || !EventTag.IsValid())
	{
		EndTask();
		return;
	}

	// 원격 플레이어의 서버 능력은 클라이언트 Notify RPC만 대기한다.
	// 따라서 같은 Notify가 서버 AnimInstance에서도 발생해도 서버 로직을 중복 실행하지 않는다.
	
	if (IsForRemoteClient())
	{
		ReplicatedGameplayEventHandle = ASC->OnReplicatedGameplayEvent.AddUObject(this, &ThisClass::HandleReplicatedGameplayEvent);
		SetWaitingOnRemotePlayerData();
		
		return;
	}

	FGameplayEventMulticastDelegate Delegate = ASC->GenericGameplayEventCallbacks.FindOrAdd(EventTag);
	LocalGameplayEventHandle = Delegate.AddUObject(this, &ThisClass::HandleLocalGameplayEvent);
}

void USpAbilityTask_WaitReplicatedGameplayEvent::OnDestroy(bool AbilityEnding)
{
	if (USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent())
	{
		if (LocalGameplayEventHandle.IsValid())
			ASC->GenericGameplayEventCallbacks.FindOrAdd(EventTag).Remove(LocalGameplayEventHandle);

		if (ReplicatedGameplayEventHandle.IsValid())
			ASC->OnReplicatedGameplayEvent.Remove(ReplicatedGameplayEventHandle);
	}

	Super::OnDestroy(AbilityEnding);
}

//  handle

void USpAbilityTask_WaitReplicatedGameplayEvent::HandleLocalGameplayEvent(const FGameplayEventData* Payload)
{
	if (!Payload)
		return;

	FGameplayEventData EventPayload = *Payload;
	EventPayload.EventTag = EventTag;

	if (IsPredictingClient())
	{
		if (USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent())
			ASC->SendReplicatedGameplayEvent_Client(GetAbilitySpecHandle(), GetActivationPredictionKey(), EventPayload);

		return;
	}

	BroadcastEvent(MoveTemp(EventPayload));
}

void USpAbilityTask_WaitReplicatedGameplayEvent::HandleReplicatedGameplayEvent(const FGameplayAbilitySpecHandle& AbilityHandle, const FPredictionKey& ActivationPredictionKey, const FGameplayEventData& Payload)
{
	if (AbilityHandle != GetAbilitySpecHandle() || ActivationPredictionKey != GetActivationPredictionKey() || Payload.EventTag != EventTag)
		return;

	ClearWaitingOnRemotePlayerData();
	BroadcastEvent(Payload);
}

void USpAbilityTask_WaitReplicatedGameplayEvent::BroadcastEvent(FGameplayEventData Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates())
		EventReceived.Broadcast(MoveTemp(Payload));

	if (bOnlyTriggerOnce)
		EndTask();
}

// node

USpAbilityTask_WaitReplicatedGameplayEvent* USpAbilityTask_WaitReplicatedGameplayEvent::WaitReplicatedGameplayEvent(UGameplayAbility* OwningAbility, FGameplayTag EventTag, bool bOnlyTriggerOnce)
{
	USpAbilityTask_WaitReplicatedGameplayEvent* Task = NewAbilityTask<USpAbilityTask_WaitReplicatedGameplayEvent>(OwningAbility);
	Task->EventTag = EventTag;
	Task->bOnlyTriggerOnce = bOnlyTriggerOnce;
	
	return Task;
}
