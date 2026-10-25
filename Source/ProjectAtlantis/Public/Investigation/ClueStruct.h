#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ClueStruct.generated.h"

// Enum for ClueType
UENUM(BlueprintType)
enum class EClueType : uint8
{
	None,
	Physical,
	Communication,
	Documentation,
	Environmental,
	Observation,
	Linking
};	

USTRUCT(BlueprintType)
struct FClue : public FTableRowBase
{
	GENERATED_BODY()
	// object name, description, and clue 'type'
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue") 
	FString ClueName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue")
	FString ClueDescription; 
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue")
	EClueType ClueType = EClueType::None;  
	// ALSO check to see if optional or mandatory 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clue")
	bool bRequired = true;
	// ALSO check to see if already discovered
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Clue")
	bool bDiscovered = false;
	// ALSO check if clue has been completed
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Clue")
	bool bCompleted = false;
};



// name, description, cluetype, discovered, completed, and ID will be in a datatable DT_Clues 