#include "Monsters/MonsterTypes.h"

namespace MonsterTags
{
    // Noise Events
    FGameplayTag Noise_Event;
    FGameplayTag Noise_Event_Loud;
    FGameplayTag Noise_Event_Quiet;
    FGameplayTag Noise_Event_ScrewTurn;
    FGameplayTag Noise_Event_Footstep_Walk;
    FGameplayTag Noise_Event_Footstep_Run;
    FGameplayTag Noise_Event_Door_Open;
    FGameplayTag Noise_Event_Door_Close;
    FGameplayTag Noise_Event_Item_Drop;
    FGameplayTag Noise_Event_Radio_Active;

    // Resources (FNAF style)
    FGameplayTag Resource_Electricity;
    FGameplayTag Resource_Oil;
    FGameplayTag Resource_Battery;
    FGameplayTag Resource_Camera;

    // Creature States
    FGameplayTag State_Idle;
    FGameplayTag State_Patrol;
    FGameplayTag State_Investigate;
    FGameplayTag State_Chase;
    FGameplayTag State_Attack;
    FGameplayTag State_Hiding;
    FGameplayTag State_Peeking;

    // Creature Types
    FGameplayTag Creature_Peeker;
    FGameplayTag Creature_LightHater;
    FGameplayTag Creature_CeilingCrawler;
    FGameplayTag Creature_GlowHider;

    // Light
    FGameplayTag Light_Source;
    FGameplayTag Light_Flashlight;
    FGameplayTag Light_Environmental;

    void InitializeTags()
    {
        // Noise Events
        Noise_Event = FGameplayTag::RequestGameplayTag("Noise.Event");
        Noise_Event_Loud = FGameplayTag::RequestGameplayTag("Noise.Event.Loud");
        Noise_Event_Quiet = FGameplayTag::RequestGameplayTag("Noise.Event.Quiet");
        Noise_Event_ScrewTurn = FGameplayTag::RequestGameplayTag("Noise.Event.ScrewTurn");
        Noise_Event_Footstep_Walk = FGameplayTag::RequestGameplayTag("Noise.Event.Footstep.Walk");
        Noise_Event_Footstep_Run = FGameplayTag::RequestGameplayTag("Noise.Event.Footstep.Run");
        Noise_Event_Door_Open = FGameplayTag::RequestGameplayTag("Noise.Event.Door.Open");
        Noise_Event_Door_Close = FGameplayTag::RequestGameplayTag("Noise.Event.Door.Close");
        Noise_Event_Item_Drop = FGameplayTag::RequestGameplayTag("Noise.Event.Item.Drop");
        Noise_Event_Radio_Active = FGameplayTag::RequestGameplayTag("Noise.Event.Radio.Active");

        // Resources
        Resource_Electricity = FGameplayTag::RequestGameplayTag("Resource.Electricity");
        Resource_Oil = FGameplayTag::RequestGameplayTag("Resource.Oil");
        Resource_Battery = FGameplayTag::RequestGameplayTag("Resource.Battery");
        Resource_Camera = FGameplayTag::RequestGameplayTag("Resource.Camera");

        // Creature States
        State_Idle = FGameplayTag::RequestGameplayTag("State.Idle");
        State_Patrol = FGameplayTag::RequestGameplayTag("State.Patrol");
        State_Investigate = FGameplayTag::RequestGameplayTag("State.Investigate");
        State_Chase = FGameplayTag::RequestGameplayTag("State.Chase");
        State_Attack = FGameplayTag::RequestGameplayTag("State.Attack");
        State_Hiding = FGameplayTag::RequestGameplayTag("State.Hiding");
        State_Peeking = FGameplayTag::RequestGameplayTag("State.Peeking");

        // Creature Types
        Creature_Peeker = FGameplayTag::RequestGameplayTag("Creature.Peeker");
        Creature_LightHater = FGameplayTag::RequestGameplayTag("Creature.LightHater");
        Creature_CeilingCrawler = FGameplayTag::RequestGameplayTag("Creature.CeilingCrawler");
        Creature_GlowHider = FGameplayTag::RequestGameplayTag("Creature.GlowHider");

        // Light
        Light_Source = FGameplayTag::RequestGameplayTag("Light.Source");
        Light_Flashlight = FGameplayTag::RequestGameplayTag("Light.Flashlight");
        Light_Environmental = FGameplayTag::RequestGameplayTag("Light.Environmental");
    }
}