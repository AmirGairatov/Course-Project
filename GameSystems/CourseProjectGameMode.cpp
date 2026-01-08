// Copyright Epic Games, Inc. All Rights Reserved.

#include "CourseProjectGameMode.h"
#include "CourseProjectCharacter.h"
#include "PlayerCharacter.h"
#include "TimerManager.h" 
#include "UObject/ConstructorHelpers.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include"CourseProjectStateBase.h"
#include "NavigationSystem.h"
#include "CourseProjectGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "CastleEnemy.h"

ACourseProjectGameMode::ACourseProjectGameMode()
{
    GameStateClass = ACourseProjectStateBase::StaticClass();
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_PlayerCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
    static ConstructorHelpers::FClassFinder<AActor> EnemyBP(TEXT("/Game/ThirdPerson/Blueprints/BP_BasicEnemy")); 
    if (EnemyBP.Succeeded())
    {
        EnemyBPClass = EnemyBP.Class;
    }
    static ConstructorHelpers::FClassFinder<AActor> CastleEnemyBP(TEXT("/Game/ThirdPerson/Blueprints/BP_CastleEnemy")); 
    if (CastleEnemyBP.Succeeded())
    {
        CastleEnemyBPClass = CastleEnemyBP.Class;
    }
    static ConstructorHelpers::FClassFinder<AActor> CrystalBP(TEXT("/Game/ThirdPerson/Blueprints/BP_Crystal")); 
    if (CastleEnemyBP.Succeeded())
    {
        CrystalBPClass = CrystalBP.Class;
    }
    static ConstructorHelpers::FClassFinder<UUserWidget> LoseWidgetBPClass(TEXT("/Game/ThirdPerson/Blueprints/WBP_GameLost"));
    if (LoseWidgetBPClass.Succeeded())
    {
        LoseWidgetClass = LoseWidgetBPClass.Class;
    }
    static ConstructorHelpers::FClassFinder<UUserWidget> WinWidgetBPClass(TEXT("/Game/ThirdPerson/Blueprints/WBP_GameWin"));
    if (WinWidgetBPClass.Succeeded())
    {
        WinWidgetClass = WinWidgetBPClass.Class;
    }

    GameModeState = TEXT("normal");

}
void ACourseProjectGameMode::BeginPlay()
{
    UE_LOG(LogTemp, Warning, TEXT("Game mod!"));
	Super::BeginPlay();
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    UCourseProjectGameInstance* GI = Cast<UCourseProjectGameInstance>(UGameplayStatics::GetGameInstance(this));
    if (GI)
    {
        
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;

        GI->HideLoadingScreen();
    }
    TArray<FVector> SpawnLocations;
    SpawnLocations.Add(FVector(12110.0f, 9210.0f, 223.395782f));
    SpawnLocations.Add(FVector(-18210.0f, 8650.0f, 3200.0f));
    SpawnLocations.Add(FVector(-14050.0f, -13220.0f, 5620.0f));
    PingInternetOnce();
    SpawnCrystals(SpawnLocations);
	//GetWorldTimerManager().SetTimer(
	//	WorldEventTimerHandle,                
	//	this,                                
	//	&ACourseProjectGameMode::GenerateWorldEvent, 
	//	7.0f,                    
	//	false                                
	//);
    TryStartWorldEventTimer(7.0f);
}

void ACourseProjectGameMode::SpawnCrystals(const TArray<FVector>& SpawnLocations)
{
    UWorld* World = GetWorld();
    if (!World || !CrystalBPClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("World or CrystalBPClass is null!"));
        return;
    }

    CrystalArray.Empty(); 

    for (const FVector& Location : SpawnLocations)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ACrystal* NewCrystal = World->SpawnActor<ACrystal>(CrystalBPClass, Location, FRotator::ZeroRotator, SpawnParams);
        if (NewCrystal)
        {
            NewCrystal->_SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            NewCrystal->_SphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
            NewCrystal->_SphereComponent->SetGenerateOverlapEvents(true);
            NewCrystal->_StaticMeshPortal->SetRelativeLocation(Location + FVector(0.0, 0.0, 3500.0));
            CrystalArray.Add(NewCrystal);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to spawn crystal at %s"), *Location.ToString());
        }
    }
}

void ACourseProjectGameMode::LogLLMConversation(const FString& Prompt, const FString& Response)
{
    FString LogEntry = TEXT("=== LLM REQUEST ===\n") + Prompt + TEXT("\n=== LLM RESPONSE ===\n") + Response + TEXT("\n---------------------\n");
    FString LogFilePath = FPaths::ProjectSavedDir() / TEXT("logs.txt");
    FFileHelper::SaveStringToFile(LogEntry, *LogFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}

void ACourseProjectGameMode::OnLLMHttpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    ACourseProjectStateBase* GS = GetWorld()->GetGameState<ACourseProjectStateBase>();
    if (!GS) return;
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("LLM API request failed"));
        GS->NumberOfLLMErrors++;
        TryStartWorldEventTimer(7.0f);
        return;
    }

    FString ResponseStr = Response->GetContentAsString();
    LogLLMConversation(LastLLMPrompt, ResponseStr);
    TSharedPtr<FJsonObject> JsonResponse;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);

    if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse LLM JSON response"));
        GS->NumberOfLLMErrors++;
        TryStartWorldEventTimer(7.0f);
        return;
    }

    if (JsonResponse->HasField(TEXT("error")))
    {
        FString ErrMsg = JsonResponse->GetObjectField(TEXT("error"))->GetStringField(TEXT("message"));
        UE_LOG(LogTemp, Error, TEXT("LLM error: %s"), *ErrMsg);
        GS->NumberOfLLMErrors++;
        TryStartWorldEventTimer(7.0f);
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
    if (JsonResponse->TryGetArrayField(TEXT("choices"), ChoicesArray) && ChoicesArray->Num() > 0)
    {
        TSharedPtr<FJsonObject> FirstChoice = (*ChoicesArray)[0]->AsObject();
        if (FirstChoice.IsValid() && FirstChoice->HasField(TEXT("message")))
        {
            TSharedPtr<FJsonObject> MessageObj = FirstChoice->GetObjectField(TEXT("message"));
            FString Content;
            if (MessageObj->TryGetStringField(TEXT("content"), Content))
            {
                UE_LOG(LogTemp, Warning, TEXT("%s"), *Content);
                HandleWorldEvent(Content);
                return;
            }
        }
    }

    UE_LOG(LogTemp, Error, TEXT("No valid content found in LLM response."));
    TryStartWorldEventTimer(7.0f);
}

void ACourseProjectGameMode::PingInternetOnce()
{
    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

    Request->SetURL(TEXT("https://api.openai.com/v1/models"));
    Request->SetVerb(TEXT("GET"));

    Request->OnProcessRequestComplete().BindLambda(
        [](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Internet ping finished. Success=%d"), bSuccess);
        }
    );

    Request->ProcessRequest();
}


void ACourseProjectGameMode::GenerateWorldEvent()
{

    //FString systemContent = TEXT(
    //    "You are a deterministic game event selector. ALWAYS output exactly ONE JSON object and NOTHING else.\n\n"

    //    "Schema - output must be one of:\n"
    //    "1) {\"event\":\"give_heal_item\",\"amount\": integer}\n"
    //    "2) {\"event\":\"spawn_enemy\",\"enemy_type\": string, \"count\": integer,\"hp\": integer,\"damage\": integer}\n"
    //    "3) {\"event\":\"spawn_hint\"}\n"
    //    "4) {\"event\":\"set_game_mode\",\"mode\": string}  // e.g. \"aggressive\" or \"normal\"\n\n"

    //    "Rules:\n"
    //    "- If player_hp much lower than player_max_hp AND heal_items < heal_items_max, output give_heal_item. Amount must not exceed heal_items_max - heal_items.\n"
    //    "- If give_heal_item is not applicable, consider spawn_hint.\n"
    //    "- If artifacts_remaining <=1 AND time_left_seconds >= 0.6 * total_time_seconds AND current game mode is not agressive, output set_game_mode with mode \"aggressive\".\n"

    //    "- NEW: spawn_enemy MUST include enemy_type.\n"
    //    "- NEW: enemy_type MUST match location:\n"
    //    "     lava_land -> \"lava_golem\"\n"
    //    "     castle_forest -> \"castle_knight\"\n\n"

    //    "- Enemy constraints: hp MUST be > player_max_hp. damage MUST be >= player_damage. All numbers integers.\n"
    //    "- Do NOT spawn new enemies if alive_enemies > 3. In that case, skip spawn_enemy and prefer other options.\n"
    //    "- Count: normally choose 1..3 enemies. If game_mode == \"aggressive\", choose 2..4 and bias to higher counts.\n"
    //    "- Aggressive scaling: when game_mode == \"aggressive\" (or immediately after set_game_mode -> \"aggressive\"), spawn_enemy should produce stronger enemies: hp >= max(player_max_hp + 100, round(base_hp * 1.3)); damage >= max(player_damage + 10, round(player_damage * 1.2)).\n"
    //    "- spawn_hint: if time_left_seconds <= 0.4 * total_time_seconds AND artifacts_remaining > 0 AND there isn't any hints exists, output spawn_hint.\n"
    //    "- If give_heal_item is not applicable and no hint/set_game_mode trigger applies, default to spawn_enemy.\n"

    //    "- Ensure you DO NOT output extra fields outside the schemas above. Use deterministic rules (temperature = 0).\n"
    //    "Priority when multiple rules apply choose by yourself.\n"
    //);
    FString systemContent = TEXT(
        "You are a deterministic game event selector. ALWAYS output exactly ONE JSON object and NOTHING else.\n\n"

        "Schema - output must be one of:\n"
        "1) {\"event\":\"give_heal_item\",\"amount\": integer}\n"
        "2) {\"event\":\"spawn_enemy\",\"enemy_type\": string, \"count\": integer,\"hp\": integer,\"damage\": integer, \"recovery_time\": number}\n"
        "3) {\"event\":\"spawn_hint\"}\n"
        "4) {\"event\":\"set_game_mode\",\"mode\": string}  // e.g. \"aggressive\" or \"normal\"\n"
        "5) {\"event\":\"buff\",\"target\": string}  // target: \"player\" or \"enemy\"\n"
        "6) {\"event\":\"respawn_buff\"}  // very rare and valuable buff\n\n"

        "Rules:\n"
        "- If player_hp much lower than player_max_hp AND heal_items < heal_items_max, output give_heal_item. Amount must not exceed heal_items_max - heal_items.\n"

        "- Respawn buff rules:\n"
        "  * Respawn buff is VERY valuable and must be used rarely.\n"
        "  * Do NOT output respawn_buff if respawn_buff == true.\n"
        "  * Respawn buff can be granted EVEN IF alive_enemies > 0.\n"
        "  * Output respawn_buff ONLY IF ALL conditions below are met:\n"
        "      - artifacts_remaining == 1\n"
        "      - game_mode == \"aggressive\"\n"
        "      - time_left_seconds < 620\n"

        "- If give_heal_item is not applicable, consider spawn_hint.\n"
        "- If artifacts_remaining <=1 AND time_left_seconds >= 0.65 * total_time_seconds AND current game mode is not agressive, output set_game_mode with mode \"aggressive\".\n"

        "- NEW: spawn_enemy MUST include enemy_type.\n"
        "- NEW: enemy_type MUST match location:\n"
        "     lava_land -> \"lava_golem\"\n"
        "     castle_forest -> \"castle_knight\"\n\n"

        "- Enemy constraints: hp MUST be > player_max_hp. damage MUST be >= player_damage. All numbers integers.\n"
        "- Do NOT spawn new enemies if alive_enemies > 0. In that case, skip spawn_enemy and prefer other options.\n"
        "- Count: normally choose 1..3 enemies. If game_mode == \"aggressive\", choose 2..4 and bias to higher counts. Do not spawn castle_knight more than 3\n"
        "- Aggressive scaling: when game_mode == \"aggressive\" (or immediately after set_game_mode -> \"aggressive\"), spawn_enemy should produce stronger enemies: hp >= max(player_max_hp + 100, round(base_hp * 1.3)); damage >= max(player_damage + 10, round(player_damage * 1.2)).\n"
        "- spawn_hint: if time_left_seconds <= 0.4 * total_time_seconds AND artifacts_remaining > 0 AND there isn't any hints exists, output spawn_hint.\n"
        "- If give_heal_item is not applicable and no hint/set_game_mode/respawn_buff trigger applies, default to spawn_enemy.\n"

        "- Enemy attack recovery_time parameter (for spawn_enemy event):\n"
        "  * Must be a number between 0.0 and 1.0, in increments of 0.1 (e.g., 0.0, 0.1, 0.2...0.9, 1.0).\n"
        "  * Determine recovery_time based on player progress/success:\n"
        "    - If player is close to finishing (artifacts_collected = 2 and time left > 480): set very low recovery_time (0.0-0.3).\n"
        "    - If player is making moderate progress (artifacts_collected >= 1): set medium recovery_time (0.4-0.6).\n"
        "    - If player is not making progress (artifacts_collected = 0): set high recovery_time (0.6-0.9).\n"
        "  * If game_mode == \"aggressive\", you may reduce recovery_time by an additional 0.1-0.2.\n"

        "- Buff event rules (when alive_enemies > 0):\n"
        "  * Consider buffing the player ({\"event\":\"buff\",\"target\":\"player\"}) if:\n"
        "    - Player has low health (player_hp < 40% of player_max_hp) AND few heal items (heal_items <= 1) AND multiple enemies are alive (alive_enemies >= 2).\n"
        "  * Consider buffing enemies ({\"event\":\"buff\",\"target\":\"enemy\"}) if:\n"
        "    - Player is doing well (player_hp > 70% of player_max_hp AND artifacts_collected >= 1) AND there is still significant time left (time_left_seconds > total_time_seconds/2).\n"
        "  * The buff event should be considered AFTER checking for give_heal_item and respawn_buff but BEFORE defaulting to spawn_enemy.\n"

        "- Ensure you DO NOT output extra fields outside the schemas above. Use deterministic rules (temperature = 0).\n"
        "Priority when multiple rules apply choose by yourself.\n"
    );


    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerPawn);

    TSharedPtr<FJsonObject> userContent = MakeShareable(new FJsonObject());

    //userContent->SetNumberField(TEXT("player_hp"), PlayerCharacter->GetHealthPoints());
    //userContent->SetNumberField(TEXT("player_max_hp"), PlayerCharacter->HealthPoints);
    //userContent->SetNumberField(TEXT("player_damage"), PlayerCharacter->GetPlayerDamage());
    //userContent->SetNumberField(TEXT("alive_enemies"), AliveEnemies); // или из GameMode
    //userContent->SetNumberField(TEXT("time_left_seconds"), 1200);
    //userContent->SetNumberField(TEXT("total_time_seconds"), 1200);
    //userContent->SetNumberField(TEXT("artifacts_remaining"), 3);
    //userContent->SetNumberField(TEXT("heal_items"), PlayerCharacter->GetCurrentHealItems());
    //userContent->SetNumberField(TEXT("heal_items_max"), PlayerCharacter->MaxHealItems);
    //userContent->SetStringField(TEXT("game_mode"), "Normal");
    //userContent->SetNumberField(TEXT("player_found_artifacts"), 0);

    ACourseProjectStateBase* MyGameState = GetWorld() ? Cast<ACourseProjectStateBase>(GetWorld()->GetGameState()) : nullptr;
    if (!MyGameState) return;

    userContent->SetNumberField(TEXT("player_hp"),PlayerCharacter->GetHealthPoints());
    userContent->SetNumberField(TEXT("player_max_hp"), PlayerCharacter->HealthPoints);
    userContent->SetNumberField(TEXT("player_damage"), PlayerCharacter->GetPlayerDamage());
    userContent->SetNumberField(TEXT("alive_enemies"), AliveEnemies.Num());
    userContent->SetNumberField(TEXT("time_left_seconds"), MyGameState->RemainingSeconds);
    userContent->SetNumberField(TEXT("total_time_seconds"), 1200);
    userContent->SetNumberField(TEXT("artifacts_remaining"), CrystalArray.Num());
    userContent->SetBoolField(TEXT("is_hint_exists"), bIsHintExists);
    userContent->SetNumberField(TEXT("heal_items"), PlayerCharacter->_HealItems);
    userContent->SetNumberField(TEXT("heal_items_max"), PlayerCharacter->MaxHealItems);
    userContent->SetStringField(TEXT("game_mode"), GameModeState);
    userContent->SetBoolField(TEXT("respawn_buff"), PlayerCharacter->bHasRespawnBuff);
    if (PlayerCharacter->InWhichLocation())
    {
        userContent->SetStringField(TEXT("player_location"), TEXT("lava_land"));
    }
    else
    {
        userContent->SetStringField(TEXT("player_location"), TEXT("castle_forest"));
    }
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writerr = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(userContent.ToSharedRef(), Writerr);
    LastLLMPrompt = OutputString;
    UE_LOG(LogTemp, Error, TEXT("Generated JSON: %s"), *OutputString);
    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

    Request->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), TEXT("Bearer YOUR_API_KEY")); 

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("model"), TEXT("gpt-4o-mini"));

    TArray<TSharedPtr<FJsonValue>> MessagesArray;

    TSharedPtr<FJsonObject> SysMsg = MakeShareable(new FJsonObject);
    SysMsg->SetStringField(TEXT("role"), TEXT("system"));
    SysMsg->SetStringField(TEXT("content"), systemContent);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(SysMsg)));

    TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject);
    UserMsg->SetStringField(TEXT("role"), TEXT("user"));
    UserMsg->SetStringField(TEXT("content"), OutputString);
    MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMsg)));

    JsonObject->SetArrayField(TEXT("messages"), MessagesArray);

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    Request->SetContentAsString(RequestBody);

    Request->OnProcessRequestComplete().BindUObject(this, &ACourseProjectGameMode::OnLLMHttpResponseReceived);
    Request->ProcessRequest();
}

void ACourseProjectGameMode::GenerateWorldEvent_Deterministic()
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    APlayerCharacter* Player = Cast<APlayerCharacter>(PlayerPawn);
    ACourseProjectStateBase* State = GetWorld() ?
        Cast<ACourseProjectStateBase>(GetWorld()->GetGameState()) : nullptr;
    if (!Player || !State) return;

    int32 playerHP = Player->GetHealthPoints();
    int32 playerMaxHP = Player->HealthPoints;
    int32 playerDamage = Player->GetPlayerDamage();
    int32 healItems = Player->_HealItems;
    int32 maxHealItems = Player->MaxHealItems;
    int32 aliveEnemies = AliveEnemies.Num();  
    int32 artifactsRemaining = CrystalArray.Num();
    bool hasRespawnBuff = Player->bHasRespawnBuff;
    bool hintExists = bIsHintExists;
    float timeLeft = State->RemainingSeconds;
    const float totalTime = 1200.0f; 

    FString currentMode = GameModeState;

    // 1. Give heal item if player is very low on HP and has heals available
    if (playerHP < playerMaxHP * 0.3f && healItems < maxHealItems)
    {
        int32 maxGive = maxHealItems - healItems;
        int32 amount = FMath::RandRange(1, maxGive);
        TSharedPtr<FJsonObject> EventJson = MakeShareable(new FJsonObject());
        EventJson->SetStringField("event", "give_heal_item");
        EventJson->SetNumberField("amount", amount);
        FString Content;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
        FJsonSerializer::Serialize(EventJson.ToSharedRef(), Writer);
        LogLLMConversation(TEXT("DETERMINISTIC_SELECTOR"), Content);
        HandleWorldEvent(Content);
        return;
    }

    // 2. Respawn buff if exactly 1 artifact left, mode is aggressive, time low, and player lacks it
    if (!hasRespawnBuff && artifactsRemaining == 1
        && currentMode == "aggressive" && timeLeft < 620.0f)
    {
        TSharedPtr<FJsonObject> EventJson = MakeShareable(new FJsonObject());
        EventJson->SetStringField("event", "respawn_buff");
        FString Content;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
        FJsonSerializer::Serialize(EventJson.ToSharedRef(), Writer);
        LogLLMConversation(TEXT("DETERMINISTIC_SELECTOR"), Content);
        HandleWorldEvent(Content);
        return;
    }

    // 3. Spawn hint if 40% time left or less, some artifacts remain, and no hint is active
    if (timeLeft <= 0.5f * totalTime && !hintExists && artifactsRemaining >= 2)
    {
        TSharedPtr<FJsonObject> EventJson = MakeShareable(new FJsonObject());
        EventJson->SetStringField("event", "spawn_hint");
        FString Content;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
        FJsonSerializer::Serialize(EventJson.ToSharedRef(), Writer);
        LogLLMConversation(TEXT("DETERMINISTIC_SELECTOR"), Content);
        HandleWorldEvent(Content);
        return;
    }

    // 4. Switch to aggressive mode if nearly all artifacts are collected and not yet aggressive
    if (artifactsRemaining <= 1 && timeLeft >= 0.6f * totalTime
        && currentMode != "aggressive")
    {
        TSharedPtr<FJsonObject> EventJson = MakeShareable(new FJsonObject());
        EventJson->SetStringField("event", "set_game_mode");
        EventJson->SetStringField("mode", "aggressive");
        FString Content;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
        FJsonSerializer::Serialize(EventJson.ToSharedRef(), Writer);
        LogLLMConversation(TEXT("DETERMINISTIC_SELECTOR"), Content);
        HandleWorldEvent(Content);
        return;
    }

    // 5. Buff events (only if enemies are currently alive)
    if (aliveEnemies > 0)
    {
        if ( healItems <= 3 && aliveEnemies >= 2)
        {
            TSharedPtr<FJsonObject> EventJson = MakeShareable(new FJsonObject());
            EventJson->SetStringField("event", "buff");
            EventJson->SetStringField("target", "player");
            FString Content;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
            FJsonSerializer::Serialize(EventJson.ToSharedRef(), Writer);
            LogLLMConversation(TEXT("DETERMINISTIC_SELECTOR"), Content);
            HandleWorldEvent(Content);
            return;
        }
        int32 initialArtifacts = 3;
        int32 artifactsCollected = artifactsRemaining;
        if (playerHP > playerMaxHP * 0.7f && artifactsCollected >= 1 && healItems>=3)
        {
            TSharedPtr<FJsonObject> EventJson = MakeShareable(new FJsonObject());
            EventJson->SetStringField("event", "buff");
            EventJson->SetStringField("target", "enemy");
            FString Content;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
            FJsonSerializer::Serialize(EventJson.ToSharedRef(), Writer);
            LogLLMConversation(TEXT("DETERMINISTIC_SELECTOR"), Content);
            HandleWorldEvent(Content);
            return;
        }
    }

    // 6. Spawn enemy (default) — only if no other event was triggered above
    // Determine enemy type by player location
    FString enemyType = Player->InWhichLocation() ? "lava_golem" : "castle_knight";

    int32 minCount = (currentMode == "aggressive") ? 2 : 1;
    int32 maxCount = (currentMode == "aggressive") ? 4 : 3;
    if (enemyType == "castle_knight")
    {
        maxCount = FMath::Min(maxCount, 3);
    }
    int32 count = FMath::RandRange(minCount, maxCount);

    int32 hp = FMath::RandRange(playerMaxHP + 1, playerMaxHP + 50);
    int32 damage = FMath::RandRange(playerDamage, playerDamage + 5);
    if (currentMode == "aggressive")
    {
        int32 thrHP = FMath::Max(playerMaxHP + 100, FMath::RoundToInt(hp * 1.3f));
        hp = FMath::Max(hp, thrHP);
        int32 thrDamage = FMath::Max(playerDamage + 10, FMath::RoundToInt(playerDamage * 1.2f));
        damage = FMath::Max(damage, thrDamage);
    }

    int32 collected = artifactsRemaining;
    float low, high;
    if (collected <= 1 && timeLeft > 480.0f)
    {
        low = 0.0f; high = 0.3f;
    }
    else if (collected <= 2)
    {
        low = 0.4f; high = 0.6f;
    }
    else
    {
        low = 0.6f; high = 0.9f;
    }
    int32 iLow = FMath::RoundToInt(low * 10);
    int32 iHigh = FMath::RoundToInt(high * 10);
    int32 step = FMath::RandRange(iLow, iHigh);
    float recoveryTime = step / 10.0f;
    if (currentMode == "aggressive")
    {
        int32 reduce = FMath::RandRange(1, 2);
        recoveryTime = FMath::Max(0.0f, recoveryTime - 0.1f * reduce);
    }

    TSharedPtr<FJsonObject> EventJson = MakeShareable(new FJsonObject());
    EventJson->SetStringField("event", "spawn_enemy");
    EventJson->SetStringField("enemy_type", enemyType);
    EventJson->SetNumberField("count", count);
    EventJson->SetNumberField("hp", hp);
    EventJson->SetNumberField("damage", damage);
    EventJson->SetNumberField("recovery_time", recoveryTime);

    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(EventJson.ToSharedRef(), Writer);
    LogLLMConversation(TEXT("DETERMINISTIC_SELECTOR"), Content);
    HandleWorldEvent(Content);
}


void ACourseProjectGameMode::HandleWorldEvent(const FString& Content)
{
    TSharedPtr<FJsonObject> JsonEvent;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    ACourseProjectStateBase* GS = GetWorld()->GetGameState<ACourseProjectStateBase>();
    if (!GS) return;
    if (!FJsonSerializer::Deserialize(Reader, JsonEvent) || !JsonEvent.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse World Event JSON"));
        GS->NumberOfLLMErrors++;
        TryStartWorldEventTimer(7.0f);
        return;
    }

    FString EventType;
    if (!JsonEvent->TryGetStringField(TEXT("event"), EventType))
    {
        UE_LOG(LogTemp, Error, TEXT("No 'event' field in World Event JSON"));
        GS->NumberOfLLMErrors++;
        TryStartWorldEventTimer(7.0f);
        return;
    }

    if (EventType == TEXT("spawn_enemy"))
    {
        int32 Count = 0, HP = 0, Damage = 0;
        float RecoveryTime = 0.5f; 
        FString EnemyType;

        if (!JsonEvent->TryGetNumberField(TEXT("count"), Count) ||
            !JsonEvent->TryGetNumberField(TEXT("hp"), HP) ||
            !JsonEvent->TryGetNumberField(TEXT("damage"), Damage) ||
            !JsonEvent->TryGetStringField(TEXT("enemy_type"), EnemyType))
        {
            UE_LOG(LogTemp, Error, TEXT("Missing parameters for spawn_enemy event"));
            GS->NumberOfLLMErrors++;
            TryStartWorldEventTimer(7.0f);
            return;
        }

        JsonEvent->TryGetNumberField(TEXT("recovery_time"), RecoveryTime);

        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
        FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

        SpawnEnemies(EnemyType, Count, HP, Damage, PlayerLocation, 150.f, RecoveryTime);
    }
    else if (EventType == TEXT("respawn_buff"))
    {
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerPawn);
        if (PlayerCharacter)
        {
            PlayerCharacter->bHasRespawnBuff = true;
        }
        TryStartWorldEventTimer(7.0f);
    }
    else if (EventType == TEXT("give_heal_item"))
    {
        int32 Amount = 0;
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerPawn);
        if (!JsonEvent->TryGetNumberField(TEXT("amount"), Amount))
        {
            UE_LOG(LogTemp, Error, TEXT("Missing 'amount' for give_heal_item event"));
            GS->NumberOfLLMErrors++;
            TryStartWorldEventTimer(7.0f);
            return;
        }
        PlayerCharacter->GiveHeal(Amount);
        GetWorldTimerManager().SetTimer(WorldEventTimerHandle, this, &ACourseProjectGameMode::GenerateWorldEvent, 10.f, false);
    }
    else if (EventType == TEXT("spawn_hint"))
    {
        SpawnHint();
    }
    else if (EventType == TEXT("set_game_mode"))
    {
        GameModeState = TEXT("aggressive");
        TryStartWorldEventTimer(7.0f);
    }
    else if (EventType == TEXT("buff"))
    {
        FString Target;
        if (!JsonEvent->TryGetStringField(TEXT("target"), Target))
        {
            UE_LOG(LogTemp, Error, TEXT("Missing 'target' for buff event"));
            GS->NumberOfLLMErrors++;
            TryStartWorldEventTimer(7.0f);
            return;
        }

        if (Target == TEXT("player"))
        {

            Buff(true);
        }
        else if (Target == TEXT("enemy"))
        {

            Buff(false);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Unknown buff target: %s"), *Target);
            GS->NumberOfLLMErrors++;
        }

        TryStartWorldEventTimer(25.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Unknown event type: %s"), *EventType);
        TryStartWorldEventTimer(7.0f);
    }
}

void ACourseProjectGameMode::OnEnemyEliminated(AActor* DestroyedActor)
{
    AliveEnemies.Remove(DestroyedActor);
    ACourseProjectStateBase* GS = GetWorld()->GetGameState<ACourseProjectStateBase>();
    if (GS)
    {
        GS->EnemiesEliminated++;
    }
    if (AliveEnemies.Num() == 0)
    {
        TryStartWorldEventTimer(12.0f);
    }
    else
    {
        TryStartWorldEventTimer(30.0f);
    }
}

void ACourseProjectGameMode::TryStartWorldEventTimer(float timer)
{
   /* if (AliveEnemies.Num() == 0)
    {
        if (!GetWorldTimerManager().IsTimerActive(WorldEventTimerHandle))
        {
            GetWorldTimerManager().SetTimer(WorldEventTimerHandle, this, &ACourseProjectGameMode::GenerateWorldEvent, 7.0f, false);
        }
    }
    else
    {
        GetWorldTimerManager().ClearTimer(WorldEventTimerHandle);
    }*/
    GetWorldTimerManager().ClearTimer(WorldEventTimerHandle);
    GetWorldTimerManager().SetTimer(WorldEventTimerHandle, this, &ACourseProjectGameMode::GenerateWorldEvent_Deterministic, timer, false);
}

void ACourseProjectGameMode::RetargetAllEnemies()
{
    ACastleEnemy* Case1 = nullptr;
    ABasicEnemy* Case2 = nullptr;
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    for (AActor* Enemy : AliveEnemies)
    {
        if (Enemy)
        {
            Case1 = Cast<ACastleEnemy>(Enemy);
            if (Case1)
            {
                /*Case1->_ChasedTarget = PlayerPawn;*/
                Case1->PrimaryActorTick.bCanEverTick = false;
                Case1->_Weapon->Destroy();
                Case1->K2_DestroyActor();
            }
            else
            {
                Case2 = Cast<ABasicEnemy>(Enemy);
                if (Case2)
                {
                    Case2->PrimaryActorTick.bCanEverTick = false;
                    Case2->K2_DestroyActor();
                    /*Case2->_ChasedTarget = PlayerPawn;*/
                }
            }
        }
    }
    AliveEnemies.Reset();
    TryStartWorldEventTimer(7.0f);
}

void ACourseProjectGameMode::Buff(bool bIsPlayer)
{
    if (bIsPlayer)
    {
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerPawn);
        if (PlayerCharacter)
        {
            PlayerCharacter->ActivateBuff();
        }
    }
    else
    {
        ACastleEnemy* Case1 = nullptr;
        ABasicEnemy* Case2 = nullptr;
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
        for (AActor* Enemy : AliveEnemies)
        {
            if (Enemy)
            {
                Case1 = Cast<ACastleEnemy>(Enemy);
                if (Case1)
                {
                    Case1->ActivateBuff();
                }
                else
                {
                    Case2 = Cast<ABasicEnemy>(Enemy);
                    if (Case2)
                    {
                        Case2->ActivateBuff();
                    }
                }
            }
        }
        TryStartWorldEventTimer(7.0f);
    }
}

void ACourseProjectGameMode::SpawnEnemies(FString EnemyType, int32 Count, int HP, int Damage, FVector Center, float Radius, float attackInterval)
{
    if (!EnemyBPClass) return;

    if (AliveEnemies.Num() > 0)
    {
        TryStartWorldEventTimer(7.0f);
        return;
        
    }

    UWorld* World = GetWorld();
    if (!World) return;

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!NavSys)
    {
        UE_LOG(LogTemp, Warning, TEXT("Navigation System не найден!"));
        return;
    }

    const float MinDistanceFromPlayer = 200.f;       
    const float MinDistanceBetweenEnemies = 150.f;  
    const int32 MaxAttemptsPerEnemy = 10;            

    TArray<FVector> UsedLocations;

    for (int32 i = 0; i < Count; ++i)
    {
        FVector SpawnLocation = Center;
        bool bFoundValidLocation = false;

        for (int32 Attempt = 0; Attempt < MaxAttemptsPerEnemy; ++Attempt)
        {
            FNavLocation NavLocation;
            bool bFound = NavSys->GetRandomReachablePointInRadius(Center, Radius, NavLocation);
            if (!bFound)
                continue;

            if (FVector::Dist(NavLocation.Location, Center) < MinDistanceFromPlayer)
                continue;

            bool bTooCloseToOthers = false;
            for (const FVector& OtherLoc : UsedLocations)
            {
                if (FVector::Dist(NavLocation.Location, OtherLoc) < MinDistanceBetweenEnemies)
                {
                    bTooCloseToOthers = true;
                    break;
                }
            }
            if (bTooCloseToOthers)
                continue;

            SpawnLocation = NavLocation.Location;
            bFoundValidLocation = true;
            break;
        }
        if (!bFoundValidLocation)
        {
            FVector RandomDir = FMath::VRand().GetSafeNormal2D();
            SpawnLocation = Center + RandomDir * MinDistanceFromPlayer;
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AActor* SpawnedEnemy = nullptr;

        if (EnemyType == TEXT("lava_golem"))
        {
            SpawnedEnemy = World->SpawnActor<AActor>(EnemyBPClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
            if (SpawnedEnemy)
            {
                ABasicEnemy* BasicEnemy = Cast<ABasicEnemy>(SpawnedEnemy);
                if (BasicEnemy)
                {
                    BasicEnemy->SetAllParams(HP, Damage, attackInterval);
                    AliveEnemies.Add(BasicEnemy);
                }
            }
        }
        else
        {
            SpawnedEnemy = World->SpawnActor<AActor>(CastleEnemyBPClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
            if (SpawnedEnemy)
            {
                ACastleEnemy* CastleEnemy = Cast<ACastleEnemy>(SpawnedEnemy);
                if (CastleEnemy)
                {
                    CastleEnemy->SetAllParams(HP, Damage, attackInterval);
                    AliveEnemies.Add(CastleEnemy);
                }
            }
        }

        UsedLocations.Add(SpawnLocation);
    }
    if (AliveEnemies.Num() > 0)
    {
        TryStartWorldEventTimer(30.0f);
    }
    else
    {
        TryStartWorldEventTimer(7.0f);
    }
}

void ACourseProjectGameMode::DecreaseArtefactAmoint()
{
    ACourseProjectStateBase* GS = GetWorld()->GetGameState<ACourseProjectStateBase>();
    if (GS)
    {
        GS->ArtefactsRemaining--;
    }
    ArtefactsRemains -= 1;
    if (ArtefactsRemains <= 0)
    {
        ShowWidget(true);
    }
}

void ACourseProjectGameMode::ShowWidget(bool isWin)
{
    if (isWin)
    {
        SpawnWidget(WinWidgetClass, WinWidgetInstance);
    }
    else
    {
        SpawnWidget(LoseWidgetClass, LoseWidgetInstance);
    }
}

void ACourseProjectGameMode::SpawnWidget(TSubclassOf<UUserWidget> WidgetClass, UUserWidget* WidgetInstance)
{
    if (WidgetClass)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            if (!WidgetInstance)
            {
                WidgetInstance = CreateWidget<UUserWidget>(PC, WidgetClass);
            }

            if (WidgetInstance && !WidgetInstance->IsInViewport())
            {
                WidgetInstance->AddToViewport();

                FInputModeUIOnly InputMode;
                InputMode.SetWidgetToFocus(WidgetInstance->TakeWidget());
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                PC->SetInputMode(InputMode);
                PC->bShowMouseCursor = true;
            }
        }
    }

    UGameplayStatics::SetGamePaused(GetWorld(), true);
}

void ACourseProjectGameMode::SpawnHint()
{
    for (ACrystal* Walk : CrystalArray)
    {
        if (Walk && !Walk->bIsHintActive)
        {
            Walk->_StaticMeshPortal->SetVisibility(true);
            Walk->bIsHintActive = true;

            bIsHintExists = true;
            break;
        }
    }
    TryStartWorldEventTimer(7.0f);
}

void ACourseProjectGameMode::SaveSessionStatsToLog()
{
    ACourseProjectStateBase* GS = GetWorld()->GetGameState<ACourseProjectStateBase>();
    if (!GS) return;

    //GS->SessionEndTime = FDateTime::UtcNow();

    FString LogEntry = FString::Printf(
        TEXT("SESSION END:\n Session time %d\nEnemies eliminated: %d\nArtefacts remains: %d\n LLM Errors: %d\n\n"),
        GS->RemainingSeconds,
        GS->EnemiesEliminated,
        GS->ArtefactsRemaining,
        GS->NumberOfLLMErrors);

    FString LogFilePath = FPaths::ProjectSavedDir() / TEXT("logs.txt");

    FFileHelper::SaveStringToFile(LogEntry, *LogFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}
