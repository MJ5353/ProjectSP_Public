#pragma once

#include "CoreMinimal.h"
#include <type_traits>

// ==================================================

template <typename TState>
class ISpState
{
public:
	virtual ~ISpState() = default;

	virtual TState GetState() const = 0;
	virtual bool CanEnter() const { return true; }
	virtual bool CanChangeTo(TState NextState) const = 0;
	virtual void Enter() { }
	virtual void Exit() { }
	virtual void Changed(TState PreviousState, TState CurrentState) { }
};

// ==================================================

// 상태 객체의 수명과 전환 순서만 관리한다.
// 상태별 규칙과 동작은 ISpState 구현체가 담당한다.

template <typename TState>
class TSpStateMachine
{
	static_assert(std::is_enum_v<TState>, "State type must be an enum.");

	TState& CurrentState;
	TUniquePtr<ISpState<TState>> ActiveState;

	bool bActive = false;
	bool bTransitioning = false;
	
public:
	explicit TSpStateMachine(TState& InCurrentState) : CurrentState(InCurrentState) { }
	
	TSpStateMachine(const TSpStateMachine&) = delete;
	TSpStateMachine& operator=(const TSpStateMachine&) = delete;
	TSpStateMachine(TSpStateMachine&&) = delete;
	TSpStateMachine& operator=(TSpStateMachine&&) = delete;

	// core
	
	bool Begin(TUniquePtr<ISpState<TState>> InitialState)
	{
		if (bActive || bTransitioning || !InitialState.IsValid() || !InitialState->CanEnter())
			return false;

		TGuardValue<bool> TransitionGuard(bTransitioning, true);
		CurrentState = InitialState->GetState();
		ActiveState = MoveTemp(InitialState);
		bActive = true;
		ActiveState->Enter();

		return true;
	}

	template <typename TStateClass, typename... TArgs>
	bool Begin(TArgs&&... Args)
	{
		return Begin(MakeUnique<TStateClass>(Forward<TArgs>(Args)...));
	}

	bool End()
	{
		if (!bActive || bTransitioning)
			return false;

		TGuardValue<bool> TransitionGuard(bTransitioning, true);
		ActiveState->Exit();
		ActiveState.Reset();
		bActive = false;

		return true;
	}

	bool Restart()
	{
		if (!bActive || bTransitioning || !ActiveState.IsValid())
			return false;

		TGuardValue<bool> TransitionGuard(bTransitioning, true);
		ActiveState->Exit();
		ActiveState->Enter();

		return true;
	}

	bool TryChangeState(TUniquePtr<ISpState<TState>> NextState)
	{
		if (!bActive || bTransitioning || !ActiveState.IsValid() || !NextState.IsValid())
			return false;

		const TState NextStateValue = NextState->GetState();
		if (CurrentState == NextStateValue ||
			!ActiveState->CanChangeTo(NextStateValue) ||
			!NextState->CanEnter())
		{
			return false;
		}

		TGuardValue<bool> TransitionGuard(bTransitioning, true);
		const TState PreviousState = CurrentState;

		ActiveState->Exit();
		CurrentState = NextStateValue;
		ActiveState = MoveTemp(NextState);
		ActiveState->Enter();
		ActiveState->Changed(PreviousState, CurrentState);

		return true;
	}

	template <typename TStateClass, typename... TArgs>
	bool TryChangeState(TArgs&&... Args)
	{
		return TryChangeState(MakeUnique<TStateClass>(Forward<TArgs>(Args)...));
	}

	// get
	
	ISpState<TState>* GetActiveState() const { return ActiveState.Get(); }

	template <typename TStateClass>
	TStateClass* GetActiveStateAs() const
	{
		return static_cast<TStateClass*>(ActiveState.Get());
	}

	TState GetCurrentState() const { return CurrentState; }
	bool IsActive() const { return bActive; }
};
