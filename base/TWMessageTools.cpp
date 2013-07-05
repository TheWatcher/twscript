
#include "TWMessageTools.h"
#include <cstdio>

// "sDarkGameModeScrMsg" (19 chars) is the longest message, with
// fSuspending as its longest field. With the '.' and nul, that's 32
const int TWMessageTools::MAX_NAMESIZE = 32;


const char* TWMessageTools::get_message_type(sScrMsg* msg)
{
    /* Okay, seriously, what the fuck. Attempting actual RTTI on msg with typeid(*msg)
     * will crash, I guess due to a corrupt vtable. At least, that's the only thing I
     * can think of that will cause that in this case. So, calling msg -> GetName()?
     * That crashes too, at least when using liblg.a compiled in gcc, again I suspect
     * because of vtable corruption or simply :fullofspiders:. However, going into the
     * "hack" vtable seems to work, because madness.
     *
     * Lasciate ogne speranza, voi ch'intrate, damnit.
     */
#ifdef _MSC_VER
    return msg -> GetName();  // No idea if this actually works in MSVC, see scrmsgs.h for more
#else
    // This is the horrible mess that seems to work fine under GCC. Don't ask me,
    // this shit is messed up.
    return msg -> persistent_hack -> thunk_GetName();
#endif
}


bool TWMessageTools::get_message_field(cMultiParm& dest, sScrMsg* msg, const char* field)
{
    // Yes, this is horrible and nasty, but stack allocation is much faster
    // than heap allocation in a std::string or cAnsiStr, and this mess is
    // going to be slower than I'd like anyway, so bleegh
    char namebuffer[MAX_NAMESIZE];

    const char* mtypename = get_message_type(msg);
    if(mtypename) {
        snprintf(namebuffer, MAX_NAMESIZE, "%s.%s", mtypename, field);

        AccessorIter iter = message_access.find(namebuffer);
        if(iter != message_access.end()) {
            (*iter -> second)(dest, msg);
            return true;
        }
    }

    return false;
}


void TWMessageTools::access_msg_from(cMultiParm& dest, sScrMsg* msg)
{
    dest = msg -> from;
}


void TWMessageTools::access_msg_to(cMultiParm& dest, sScrMsg* msg)
{
    dest = msg -> to;
}


void TWMessageTools::access_msg_message(cMultiParm& dest, sScrMsg* msg)
{
    dest = msg -> message;
}


void TWMessageTools::access_msg_time(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(msg -> time);
}


void TWMessageTools::access_msg_flags(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(msg -> flags);
}


void TWMessageTools::access_msg_data(cMultiParm& dest, sScrMsg* msg)
{
    dest = msg -> data;
}


void TWMessageTools::access_msg_data2(cMultiParm& dest, sScrMsg* msg)
{
    dest = msg -> data2;
}


void TWMessageTools::access_msg_data3(cMultiParm& dest, sScrMsg* msg)
{
    dest = msg -> data3;
}


void TWMessageTools::access_sim_starting(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sSimMsg*>(msg) -> fStarting);
}


void TWMessageTools::access_dgmc_resuming(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sDarkGameModeScrMsg*>(msg) -> fResuming);
}


void TWMessageTools::access_dgmc_suspending(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sDarkGameModeScrMsg*>(msg) -> fSuspending);
}


void TWMessageTools::access_aimc_mode(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sAIModeChangeMsg*>(msg) -> mode);
}


void TWMessageTools::access_aimc_previousmode(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sAIModeChangeMsg*>(msg) -> previous_mode);
}


void TWMessageTools::access_aialertness_level(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sAIAlertnessMsg*>(msg) -> level);
}


void TWMessageTools::access_aialertness_oldlevel(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sAIAlertnessMsg*>(msg) -> oldLevel);
}


void TWMessageTools::access_aihighalert_level(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sAIHighAlertMsg*>(msg) -> level);
}


void TWMessageTools::access_aihighalert_oldlevel(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sAIHighAlertMsg*>(msg) -> oldLevel);
}


void TWMessageTools::access_airesult_action(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sAIResultMsg*>(msg) -> action);
}


void TWMessageTools::access_airesult_result(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sAIResultMsg*>(msg) -> result);
}


void TWMessageTools::access_airesult_resultdata(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sAIResultMsg*>(msg) -> result_data;
}


void TWMessageTools::access_aiobjactres_target(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sAIObjActResultMsg*>(msg) -> target;
}


void TWMessageTools::access_aipatrolpoint_obj(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sAIPatrolPointMsg*>(msg) -> patrolObj;
}


void TWMessageTools::access_aisignal_signal(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sAISignalMsg*>(msg) -> signal;
}


void TWMessageTools::access_attack_weapon(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sAttackMsg*>(msg) -> weapon;
}


void TWMessageTools::access_combine_combiner(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sCombineScrMsg*>(msg) -> combiner;
}


void TWMessageTools::access_contained_event(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sContainedScrMsg*>(msg) -> event;
}


void TWMessageTools::access_contained_container(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sContainedScrMsg*>(msg) -> container;
}


void TWMessageTools::access_container_event(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sContainerScrMsg*>(msg) -> event;
}


void TWMessageTools::access_container_containee(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sContainerScrMsg*>(msg) -> containee;
}


void TWMessageTools::access_damage_kind(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sDamageScrMsg*>(msg) -> kind;
}


void TWMessageTools::access_damage_damage(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sDamageScrMsg*>(msg) -> damage;
}


void TWMessageTools::access_damage_culprit(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sDamageScrMsg*>(msg) -> culprit;
}


void TWMessageTools::access_difficulty_difficulty(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sDiffScrMsg*>(msg) -> difficulty;
}


void TWMessageTools::access_door_actiontype(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sDoorMsg*>(msg) -> ActionType);
}


void TWMessageTools::access_door_prevactiontype(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sDoorMsg*>(msg) -> PrevActionType);
}


#if (_DARKGAME == 3) || ((_DARKGAME == 2) && (_NETWORKING == 1))
void TWMessageTools::access_door_isproxy(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sDoorMsg*>(msg) -> IsProxy);
}
#endif

void TWMessageTools::access_frob_srcobj(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sFrobMsg*>(msg) -> SrcObjId;
}


void TWMessageTools::access_frob_dstobj(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sFrobMsg*>(msg) -> DstObjId;
}


void TWMessageTools::access_frob_frobber(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sFrobMsg*>(msg) -> Frobber;
}


void TWMessageTools::access_frob_srcloc(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sFrobMsg*>(msg) -> SrcLoc);
}


void TWMessageTools::access_frob_dstloc(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sFrobMsg*>(msg) -> DstLoc);
}


void TWMessageTools::access_frob_sec(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sFrobMsg*>(msg) -> Sec;
}


void TWMessageTools::access_frob_abort(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sFrobMsg*>(msg) -> Abort);
}


void TWMessageTools::access_body_action(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sBodyMsg*>(msg) -> ActionType);
}


void TWMessageTools::access_body_motion(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sBodyMsg*>(msg) -> MotionName;
}


void TWMessageTools::access_body_flagvalue(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sBodyMsg*>(msg) -> FlagValue;
}


void TWMessageTools::access_pick_prevstate(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPickStateScrMsg*>(msg) -> PrevState;
}


void TWMessageTools::access_pick_newstate(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPickStateScrMsg*>(msg) -> NewState;
}


void TWMessageTools::access_phys_submod(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> Submod;
}


void TWMessageTools::access_phys_collobj(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> collObj;
}


void TWMessageTools::access_phys_colltype(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sPhysMsg*>(msg) -> collType);
}


void TWMessageTools::access_phys_collsubmod(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> collSubmod;
}


void TWMessageTools::access_phys_collmomentum(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> collMomentum;
}


void TWMessageTools::access_phys_collnormal(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> collNormal;
}


void TWMessageTools::access_phys_collpt(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> collPt;
}


void TWMessageTools::access_phys_contacttype(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sPhysMsg*>(msg) -> contactType);
}


void TWMessageTools::access_phys_contactobj(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> contactObj;
}


void TWMessageTools::access_phys_contactsubmod(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> contactSubmod;
}


void TWMessageTools::access_phys_transobj(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> transObj;
}


void TWMessageTools::access_phys_transsubmod(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sPhysMsg*>(msg) -> transSubmod;
}


void TWMessageTools::access_report_warnlevel(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sReportMsg*>(msg) -> WarnLevel;
}


void TWMessageTools::access_report_flags(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sReportMsg*>(msg) -> Flags;
}


void TWMessageTools::access_report_type(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sReportMsg*>(msg) -> Types;
}


void TWMessageTools::access_report_textbuffer(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sReportMsg*>(msg) -> TextBuffer;
}


void TWMessageTools::access_room_fromobjid(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sRoomMsg*>(msg) -> FromObjId;
}


void TWMessageTools::access_room_toobjid(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sRoomMsg*>(msg) -> ToObjId;
}


void TWMessageTools::access_room_moveobjid(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sRoomMsg*>(msg) -> MoveObjId;
}


void TWMessageTools::access_room_objtype(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sRoomMsg*>(msg) -> ObjType);
}


void TWMessageTools::access_room_transitiontype(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sRoomMsg*>(msg) -> TransitionType);
}


void TWMessageTools::access_slay_culprit(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sSlayMsg*>(msg) -> culprit;
}


void TWMessageTools::access_slay_kind(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sSlayMsg*>(msg) -> kind;
}


void TWMessageTools::access_schemadone_coords(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sSchemaDoneMsg*>(msg) -> coordinates;
}


void TWMessageTools::access_schemadone_target(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sSchemaDoneMsg*>(msg) -> targetObject;
}


void TWMessageTools::access_schemadone_name(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sSchemaDoneMsg*>(msg) -> name;
}


void TWMessageTools::access_sounddone_coords(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sSoundDoneMsg*>(msg) -> coordinates;
}


void TWMessageTools::access_sounddone_target(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sSoundDoneMsg*>(msg) -> targetObject;
}


void TWMessageTools::access_sounddone_name(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sSoundDoneMsg*>(msg) -> name;
}


void TWMessageTools::access_stim_stimulus(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sStimMsg*>(msg) -> stimulus);
}


void TWMessageTools::access_stim_intensity(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sStimMsg*>(msg) -> intensity;
}


void TWMessageTools::access_stim_sensor(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sStimMsg*>(msg) -> sensor;
}


void TWMessageTools::access_stim_source(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sStimMsg*>(msg) -> source;
}


void TWMessageTools::access_timer_name(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sScrTimerMsg*>(msg) -> name;
}


void TWMessageTools::access_tweq_type(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sTweqMsg*>(msg) -> Type);
}


void TWMessageTools::access_tweq_op(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sTweqMsg*>(msg) -> Op);
}


void TWMessageTools::access_tweq_dir(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sTweqMsg*>(msg) -> Dir);
}


void TWMessageTools::access_waypoint_mterr(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sWaypointMsg*>(msg) -> moving_terrain;
}


void TWMessageTools::access_movingterr_waypoint(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sMovingTerrainMsg*>(msg) -> waypoint;
}


void TWMessageTools::access_quest_name(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sQuestMsg*>(msg) -> m_pName;
}


void TWMessageTools::access_quest_oldvalue(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sQuestMsg*>(msg) -> m_oldValue;
}


void TWMessageTools::access_quest_newvalue(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sQuestMsg*>(msg) -> m_newValue;
}


void TWMessageTools::access_mediumtrans_from(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sMediumTransMsg*>(msg) -> nFromType;
}


void TWMessageTools::access_mediumtrans_to(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sMediumTransMsg*>(msg) -> nToType;
}


void TWMessageTools::access_yorno_yorn(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<int>(static_cast<sYorNMsg*>(msg) -> YorN);
}


void TWMessageTools::access_keypad_code(cMultiParm& dest, sScrMsg* msg)
{
    dest = static_cast<sKeypadMsg*>(msg) -> code;
}


AccessorMap TWMessageTools::message_access = {
    { "sScrMsg.from"                   , TWMessageTools::access_msg_from             },
    { "sScrMsg.to"                     , TWMessageTools::access_msg_to               },
    { "sScrMsg.message"                , TWMessageTools::access_msg_message          },
    { "sScrMsg.time"                   , TWMessageTools::access_msg_time             },
    { "sScrMsg.flags"                  , TWMessageTools::access_msg_flags            },
    { "sScrMsg.data"                   , TWMessageTools::access_msg_data             },
    { "sScrMsg.data2"                  , TWMessageTools::access_msg_data2            },
    { "sScrMsg.data3"                  , TWMessageTools::access_msg_data3            },

    { "sSimMsg.from"                   , TWMessageTools::access_msg_from             },
    { "sSimMsg.to"                     , TWMessageTools::access_msg_to               },
    { "sSimMsg.message"                , TWMessageTools::access_msg_message          },
    { "sSimMsg.time"                   , TWMessageTools::access_msg_time             },
    { "sSimMsg.flags"                  , TWMessageTools::access_msg_flags            },
    { "sSimMsg.data"                   , TWMessageTools::access_msg_data             },
    { "sSimMsg.data2"                  , TWMessageTools::access_msg_data2            },
    { "sSimMsg.data3"                  , TWMessageTools::access_msg_data3            },
    { "sSimMsg.fStarting"              , TWMessageTools::access_sim_starting         },

    { "sDarkGameModeScrMsg.from"       , TWMessageTools::access_msg_from             },
    { "sDarkGameModeScrMsg.to"         , TWMessageTools::access_msg_to               },
    { "sDarkGameModeScrMsg.message"    , TWMessageTools::access_msg_message          },
    { "sDarkGameModeScrMsg.time"       , TWMessageTools::access_msg_time             },
    { "sDarkGameModeScrMsg.flags"      , TWMessageTools::access_msg_flags            },
    { "sDarkGameModeScrMsg.data"       , TWMessageTools::access_msg_data             },
    { "sDarkGameModeScrMsg.data2"      , TWMessageTools::access_msg_data2            },
    { "sDarkGameModeScrMsg.data3"      , TWMessageTools::access_msg_data3            },
    { "sDarkGameModeScrMsg.fResuming"  , TWMessageTools::access_dgmc_resuming        },
    { "sDarkGameModeScrMsg.fSuspending", TWMessageTools::access_dgmc_suspending      },

    { "sAIModeChangeMsg.from"          , TWMessageTools::access_msg_from             },
    { "sAIModeChangeMsg.to"            , TWMessageTools::access_msg_to               },
    { "sAIModeChangeMsg.message"       , TWMessageTools::access_msg_message          },
    { "sAIModeChangeMsg.time"          , TWMessageTools::access_msg_time             },
    { "sAIModeChangeMsg.flags"         , TWMessageTools::access_msg_flags            },
    { "sAIModeChangeMsg.data"          , TWMessageTools::access_msg_data             },
    { "sAIModeChangeMsg.data2"         , TWMessageTools::access_msg_data2            },
    { "sAIModeChangeMsg.data3"         , TWMessageTools::access_msg_data3            },
    { "sAIModeChangeMsg.mode"          , TWMessageTools::access_aimc_mode            },
    { "sAIModeChangeMsg.previous_mode" , TWMessageTools::access_aimc_previousmode    },

    { "sAIAlertnessMsg.from"           , TWMessageTools::access_msg_from             },
    { "sAIAlertnessMsg.to"             , TWMessageTools::access_msg_to               },
    { "sAIAlertnessMsg.message"        , TWMessageTools::access_msg_message          },
    { "sAIAlertnessMsg.time"           , TWMessageTools::access_msg_time             },
    { "sAIAlertnessMsg.flags"          , TWMessageTools::access_msg_flags            },
    { "sAIAlertnessMsg.data"           , TWMessageTools::access_msg_data             },
    { "sAIAlertnessMsg.data2"          , TWMessageTools::access_msg_data2            },
    { "sAIAlertnessMsg.data3"          , TWMessageTools::access_msg_data3            },
    { "sAIAlertnessMsg.level"          , TWMessageTools::access_aialertness_level    },
    { "sAIAlertnessMsg.oldLevel"       , TWMessageTools::access_aialertness_oldlevel },

    { "sAIHighAlertMsg.from"           , TWMessageTools::access_msg_from             },
    { "sAIHighAlertMsg.to"             , TWMessageTools::access_msg_to               },
    { "sAIHighAlertMsg.message"        , TWMessageTools::access_msg_message          },
    { "sAIHighAlertMsg.time"           , TWMessageTools::access_msg_time             },
    { "sAIHighAlertMsg.flags"          , TWMessageTools::access_msg_flags            },
    { "sAIHighAlertMsg.data"           , TWMessageTools::access_msg_data             },
    { "sAIHighAlertMsg.data2"          , TWMessageTools::access_msg_data2            },
    { "sAIHighAlertMsg.data3"          , TWMessageTools::access_msg_data3            },
    { "sAIHighAlertMsg.level"          , TWMessageTools::access_aihighalert_level    },
    { "sAIHighAlertMsg.oldLevel"       , TWMessageTools::access_aihighalert_oldlevel },

    { "sAIResultMsg.from"              , TWMessageTools::access_msg_from             },
    { "sAIResultMsg.to"                , TWMessageTools::access_msg_to               },
    { "sAIResultMsg.message"           , TWMessageTools::access_msg_message          },
    { "sAIResultMsg.time"              , TWMessageTools::access_msg_time             },
    { "sAIResultMsg.flags"             , TWMessageTools::access_msg_flags            },
    { "sAIResultMsg.data"              , TWMessageTools::access_msg_data             },
    { "sAIResultMsg.data2"             , TWMessageTools::access_msg_data2            },
    { "sAIResultMsg.data3"             , TWMessageTools::access_msg_data3            },
    { "sAIResultMsg.action"            , TWMessageTools::access_airesult_action      },
    { "sAIResultMsg.result"            , TWMessageTools::access_airesult_result      },
    { "sAIResultMsg.result_data"       , TWMessageTools::access_airesult_resultdata  },

    { "sAIObjActResultMsg.from"        , TWMessageTools::access_msg_from             },
    { "sAIObjActResultMsg.to"          , TWMessageTools::access_msg_to               },
    { "sAIObjActResultMsg.message"     , TWMessageTools::access_msg_message          },
    { "sAIObjActResultMsg.time"        , TWMessageTools::access_msg_time             },
    { "sAIObjActResultMsg.flags"       , TWMessageTools::access_msg_flags            },
    { "sAIObjActResultMsg.data"        , TWMessageTools::access_msg_data             },
    { "sAIObjActResultMsg.data2"       , TWMessageTools::access_msg_data2            },
    { "sAIObjActResultMsg.data3"       , TWMessageTools::access_msg_data3            },
    { "sAIObjActResultMsg.target"      , TWMessageTools::access_aiobjactres_target   },

    { "sAIPatrolPointMsg.from"         , TWMessageTools::access_msg_from             },
    { "sAIPatrolPointMsg.to"           , TWMessageTools::access_msg_to               },
    { "sAIPatrolPointMsg.message"      , TWMessageTools::access_msg_message          },
    { "sAIPatrolPointMsg.time"         , TWMessageTools::access_msg_time             },
    { "sAIPatrolPointMsg.flags"        , TWMessageTools::access_msg_flags            },
    { "sAIPatrolPointMsg.data"         , TWMessageTools::access_msg_data             },
    { "sAIPatrolPointMsg.data2"        , TWMessageTools::access_msg_data2            },
    { "sAIPatrolPointMsg.data3"        , TWMessageTools::access_msg_data3            },
    { "sAIPatrolPointMsg.patrolObj"    , TWMessageTools::access_aipatrolpoint_obj    },

    { "sAISignalMsg.from"              , TWMessageTools::access_msg_from             },
    { "sAISignalMsg.to"                , TWMessageTools::access_msg_to               },
    { "sAISignalMsg.message"           , TWMessageTools::access_msg_message          },
    { "sAISignalMsg.time"              , TWMessageTools::access_msg_time             },
    { "sAISignalMsg.flags"             , TWMessageTools::access_msg_flags            },
    { "sAISignalMsg.data"              , TWMessageTools::access_msg_data             },
    { "sAISignalMsg.data2"             , TWMessageTools::access_msg_data2            },
    { "sAISignalMsg.data3"             , TWMessageTools::access_msg_data3            },
    { "sAISignalMsg.signal"            , TWMessageTools::access_aisignal_signal      },

    { "sAttackMsg.from"                , TWMessageTools::access_msg_from             },
    { "sAttackMsg.to"                  , TWMessageTools::access_msg_to               },
    { "sAttackMsg.message"             , TWMessageTools::access_msg_message          },
    { "sAttackMsg.time"                , TWMessageTools::access_msg_time             },
    { "sAttackMsg.flags"               , TWMessageTools::access_msg_flags            },
    { "sAttackMsg.data"                , TWMessageTools::access_msg_data             },
    { "sAttackMsg.data2"               , TWMessageTools::access_msg_data2            },
    { "sAttackMsg.data3"               , TWMessageTools::access_msg_data3            },
    { "sAttackMsg.weapon"              , TWMessageTools::access_attack_weapon        },

    { "sCombineScrMsg.from"            , TWMessageTools::access_msg_from             },
    { "sCombineScrMsg.to"              , TWMessageTools::access_msg_to               },
    { "sCombineScrMsg.message"         , TWMessageTools::access_msg_message          },
    { "sCombineScrMsg.time"            , TWMessageTools::access_msg_time             },
    { "sCombineScrMsg.flags"           , TWMessageTools::access_msg_flags            },
    { "sCombineScrMsg.data"            , TWMessageTools::access_msg_data             },
    { "sCombineScrMsg.data2"           , TWMessageTools::access_msg_data2            },
    { "sCombineScrMsg.data3"           , TWMessageTools::access_msg_data3            },
    { "sCombineScrMsg.combiner"        , TWMessageTools::access_combine_combiner     },

    { "sContainedScrMsg.from"          , TWMessageTools::access_msg_from             },
    { "sContainedScrMsg.to"            , TWMessageTools::access_msg_to               },
    { "sContainedScrMsg.message"       , TWMessageTools::access_msg_message          },
    { "sContainedScrMsg.time"          , TWMessageTools::access_msg_time             },
    { "sContainedScrMsg.flags"         , TWMessageTools::access_msg_flags            },
    { "sContainedScrMsg.data"          , TWMessageTools::access_msg_data             },
    { "sContainedScrMsg.data2"         , TWMessageTools::access_msg_data2            },
    { "sContainedScrMsg.data3"         , TWMessageTools::access_msg_data3            },
    { "sContainedScrMsg.event"         , TWMessageTools::access_contained_event      },
    { "sContainedScrMsg.container"     , TWMessageTools::access_contained_container  },

    { "sContainerScrMsg.from"          , TWMessageTools::access_msg_from             },
    { "sContainerScrMsg.to"            , TWMessageTools::access_msg_to               },
    { "sContainerScrMsg.message"       , TWMessageTools::access_msg_message          },
    { "sContainerScrMsg.time"          , TWMessageTools::access_msg_time             },
    { "sContainerScrMsg.flags"         , TWMessageTools::access_msg_flags            },
    { "sContainerScrMsg.data"          , TWMessageTools::access_msg_data             },
    { "sContainerScrMsg.data2"         , TWMessageTools::access_msg_data2            },
    { "sContainerScrMsg.data3"         , TWMessageTools::access_msg_data3            },
    { "sContainerScrMsg.event"         , TWMessageTools::access_container_event      },
    { "sContainerScrMsg.container"     , TWMessageTools::access_container_containee  },

    { "sDamageScrMsg.from"             , TWMessageTools::access_msg_from             },
    { "sDamageScrMsg.to"               , TWMessageTools::access_msg_to               },
    { "sDamageScrMsg.message"          , TWMessageTools::access_msg_message          },
    { "sDamageScrMsg.time"             , TWMessageTools::access_msg_time             },
    { "sDamageScrMsg.flags"            , TWMessageTools::access_msg_flags            },
    { "sDamageScrMsg.data"             , TWMessageTools::access_msg_data             },
    { "sDamageScrMsg.data2"            , TWMessageTools::access_msg_data2            },
    { "sDamageScrMsg.data3"            , TWMessageTools::access_msg_data3            },
    { "sDamageScrMsg.kind"             , TWMessageTools::access_damage_kind          },
    { "sDamageScrMsg.damage"           , TWMessageTools::access_damage_damage        },
    { "sDamageScrMsg.culprit"          , TWMessageTools::access_damage_culprit       },

    { "sDiffScrMsg.from"               , TWMessageTools::access_msg_from             },
    { "sDiffScrMsg.to"                 , TWMessageTools::access_msg_to               },
    { "sDiffScrMsg.message"            , TWMessageTools::access_msg_message          },
    { "sDiffScrMsg.time"               , TWMessageTools::access_msg_time             },
    { "sDiffScrMsg.flags"              , TWMessageTools::access_msg_flags            },
    { "sDiffScrMsg.data"               , TWMessageTools::access_msg_data             },
    { "sDiffScrMsg.data2"              , TWMessageTools::access_msg_data2            },
    { "sDiffScrMsg.data3"              , TWMessageTools::access_msg_data3            },
    { "sDiffScrMsg.difficulty"         , TWMessageTools::access_difficulty_difficulty},

    { "sDoorMsg.from"                  , TWMessageTools::access_msg_from             },
    { "sDoorMsg.to"                    , TWMessageTools::access_msg_to               },
    { "sDoorMsg.message"               , TWMessageTools::access_msg_message          },
    { "sDoorMsg.time"                  , TWMessageTools::access_msg_time             },
    { "sDoorMsg.flags"                 , TWMessageTools::access_msg_flags            },
    { "sDoorMsg.data"                  , TWMessageTools::access_msg_data             },
    { "sDoorMsg.data2"                 , TWMessageTools::access_msg_data2            },
    { "sDoorMsg.data3"                 , TWMessageTools::access_msg_data3            },
    { "sDoorMsg.ActionType"            , TWMessageTools::access_door_actiontype      },
    { "sDoorMsg.PrevActionType"        , TWMessageTools::access_door_prevactiontype  },
#if (_DARKGAME == 3) || ((_DARKGAME == 2) && (_NETWORKING == 1))
    { "sDoorMsg.IsProxy"               , TWMessageTools::access_door_isproxy         },
#endif

    { "sFrobMsg.from"                  , TWMessageTools::access_msg_from             },
    { "sFrobMsg.to"                    , TWMessageTools::access_msg_to               },
    { "sFrobMsg.message"               , TWMessageTools::access_msg_message          },
    { "sFrobMsg.time"                  , TWMessageTools::access_msg_time             },
    { "sFrobMsg.flags"                 , TWMessageTools::access_msg_flags            },
    { "sFrobMsg.data"                  , TWMessageTools::access_msg_data             },
    { "sFrobMsg.data2"                 , TWMessageTools::access_msg_data2            },
    { "sFrobMsg.data3"                 , TWMessageTools::access_msg_data3            },
    { "sFrobMsg.SrcObjId"              , TWMessageTools::access_frob_srcobj          },
    { "sFrobMsg.DstObjId"              , TWMessageTools::access_frob_dstobj          },
    { "sFrobMsg.Frobber"               , TWMessageTools::access_frob_frobber         },
    { "sFrobMsg.SrcLoc"                , TWMessageTools::access_frob_srcloc          },
    { "sFrobMsg.DstLoc"                , TWMessageTools::access_frob_dstloc          },
    { "sFrobMsg.Sec"                   , TWMessageTools::access_frob_sec             },
    { "sFrobMsg.Abort"                 , TWMessageTools::access_frob_abort           },

    { "sBodyMsg.from"                  , TWMessageTools::access_msg_from             },
    { "sBodyMsg.to"                    , TWMessageTools::access_msg_to               },
    { "sBodyMsg.message"               , TWMessageTools::access_msg_message          },
    { "sBodyMsg.time"                  , TWMessageTools::access_msg_time             },
    { "sBodyMsg.flags"                 , TWMessageTools::access_msg_flags            },
    { "sBodyMsg.data"                  , TWMessageTools::access_msg_data             },
    { "sBodyMsg.data2"                 , TWMessageTools::access_msg_data2            },
    { "sBodyMsg.data3"                 , TWMessageTools::access_msg_data3            },
    { "sBodyMsg.ActionType"            , TWMessageTools::access_body_action          },
    { "sBodyMsg.MotionName"            , TWMessageTools::access_body_motion          },
    { "sBodyMsg.FlagValue"             , TWMessageTools::access_body_flagvalue       },

    { "sPickStateScrMsg.from"          , TWMessageTools::access_msg_from             },
    { "sPickStateScrMsg.to"            , TWMessageTools::access_msg_to               },
    { "sPickStateScrMsg.message"       , TWMessageTools::access_msg_message          },
    { "sPickStateScrMsg.time"          , TWMessageTools::access_msg_time             },
    { "sPickStateScrMsg.flags"         , TWMessageTools::access_msg_flags            },
    { "sPickStateScrMsg.data"          , TWMessageTools::access_msg_data             },
    { "sPickStateScrMsg.data2"         , TWMessageTools::access_msg_data2            },
    { "sPickStateScrMsg.data3"         , TWMessageTools::access_msg_data3            },
    { "sPickStateScrMsg.PrevState"     , TWMessageTools::access_pick_prevstate       },
    { "sPickStateScrMsg.NewState"      , TWMessageTools::access_pick_newstate        },

    { "sPhysMsg.from"                  , TWMessageTools::access_msg_from             },
    { "sPhysMsg.to"                    , TWMessageTools::access_msg_to               },
    { "sPhysMsg.message"               , TWMessageTools::access_msg_message          },
    { "sPhysMsg.time"                  , TWMessageTools::access_msg_time             },
    { "sPhysMsg.flags"                 , TWMessageTools::access_msg_flags            },
    { "sPhysMsg.data"                  , TWMessageTools::access_msg_data             },
    { "sPhysMsg.data2"                 , TWMessageTools::access_msg_data2            },
    { "sPhysMsg.data3"                 , TWMessageTools::access_msg_data3            },
    { "sPhysMsg.Submod"                , TWMessageTools::access_phys_submod          },
    { "sPhysMsg.collType"              , TWMessageTools::access_phys_colltype        },
    { "sPhysMsg.collObj"               , TWMessageTools::access_phys_collobj         },
    { "sPhysMsg.collSubmod"            , TWMessageTools::access_phys_collsubmod      },
    { "sPhysMsg.collMomentum"          , TWMessageTools::access_phys_collmomentum    },
    { "sPhysMsg.collNormal"            , TWMessageTools::access_phys_collnormal      },
    { "sPhysMsg.collPt"                , TWMessageTools::access_phys_collpt          },
    { "sPhysMsg.contactType"           , TWMessageTools::access_phys_contacttype     },
    { "sPhysMsg.contactObj"            , TWMessageTools::access_phys_contactobj      },
    { "sPhysMsg.contactSubmod"         , TWMessageTools::access_phys_contactsubmod   },
    { "sPhysMsg.transObj"              , TWMessageTools::access_phys_transobj        },
    { "sPhysMsg.transSubmod"           , TWMessageTools::access_phys_transsubmod     },

    { "sReportMsg.from"                , TWMessageTools::access_msg_from             },
    { "sReportMsg.to"                  , TWMessageTools::access_msg_to               },
    { "sReportMsg.message"             , TWMessageTools::access_msg_message          },
    { "sReportMsg.time"                , TWMessageTools::access_msg_time             },
    { "sReportMsg.flags"               , TWMessageTools::access_msg_flags            },
    { "sReportMsg.data"                , TWMessageTools::access_msg_data             },
    { "sReportMsg.data2"               , TWMessageTools::access_msg_data2            },
    { "sReportMsg.data3"               , TWMessageTools::access_msg_data3            },
    { "sReportMsg.WarnLevel"           , TWMessageTools::access_report_warnlevel     },
    { "sReportMsg.Flags"               , TWMessageTools::access_report_flags         },
    { "sReportMsg.Type"                , TWMessageTools::access_report_type          },
    { "sReportMsg.TextBuffer"          , TWMessageTools::access_report_textbuffer    },

    { "sRoomMsg.from"                  , TWMessageTools::access_msg_from             },
    { "sRoomMsg.to"                    , TWMessageTools::access_msg_to               },
    { "sRoomMsg.message"               , TWMessageTools::access_msg_message          },
    { "sRoomMsg.time"                  , TWMessageTools::access_msg_time             },
    { "sRoomMsg.flags"                 , TWMessageTools::access_msg_flags            },
    { "sRoomMsg.data"                  , TWMessageTools::access_msg_data             },
    { "sRoomMsg.data2"                 , TWMessageTools::access_msg_data2            },
    { "sRoomMsg.data3"                 , TWMessageTools::access_msg_data3            },
    { "sRoomMsg.FromObjId"             , TWMessageTools::access_room_fromobjid       },
    { "sRoomMsg.ToObjId"               , TWMessageTools::access_room_toobjid         },
    { "sRoomMsg.MoveObjId"             , TWMessageTools::access_room_moveobjid       },
    { "sRoomMsg.ObjType"               , TWMessageTools::access_room_objtype         },
    { "sRoomMsg.TransitionType"        , TWMessageTools::access_room_transitiontype  },

    { "sSlayMsg.from"                  , TWMessageTools::access_msg_from             },
    { "sSlayMsg.to"                    , TWMessageTools::access_msg_to               },
    { "sSlayMsg.message"               , TWMessageTools::access_msg_message          },
    { "sSlayMsg.time"                  , TWMessageTools::access_msg_time             },
    { "sSlayMsg.flags"                 , TWMessageTools::access_msg_flags            },
    { "sSlayMsg.data"                  , TWMessageTools::access_msg_data             },
    { "sSlayMsg.data2"                 , TWMessageTools::access_msg_data2            },
    { "sSlayMsg.data3"                 , TWMessageTools::access_msg_data3            },
    { "sSlayMsg.culprit"               , TWMessageTools::access_slay_culprit         },
    { "sSlayMsg.kind"                  , TWMessageTools::access_slay_kind            },

    { "sSchemaDoneMsg.from"            , TWMessageTools::access_msg_from             },
    { "sSchemaDoneMsg.to"              , TWMessageTools::access_msg_to               },
    { "sSchemaDoneMsg.message"         , TWMessageTools::access_msg_message          },
    { "sSchemaDoneMsg.time"            , TWMessageTools::access_msg_time             },
    { "sSchemaDoneMsg.flags"           , TWMessageTools::access_msg_flags            },
    { "sSchemaDoneMsg.data"            , TWMessageTools::access_msg_data             },
    { "sSchemaDoneMsg.data2"           , TWMessageTools::access_msg_data2            },
    { "sSchemaDoneMsg.data3"           , TWMessageTools::access_msg_data3            },
    { "sSchemaDoneMsg.coordinates"     , TWMessageTools::access_schemadone_coords    },
    { "sSchemaDoneMsg.targetObject"    , TWMessageTools::access_schemadone_target    },
    { "sSchemaDoneMsg.name"            , TWMessageTools::access_schemadone_name      },

    { "sSoundDoneMsg.from"             , TWMessageTools::access_msg_from             },
    { "sSoundDoneMsg.to"               , TWMessageTools::access_msg_to               },
    { "sSoundDoneMsg.message"          , TWMessageTools::access_msg_message          },
    { "sSoundDoneMsg.time"             , TWMessageTools::access_msg_time             },
    { "sSoundDoneMsg.flags"            , TWMessageTools::access_msg_flags            },
    { "sSoundDoneMsg.data"             , TWMessageTools::access_msg_data             },
    { "sSoundDoneMsg.data2"            , TWMessageTools::access_msg_data2            },
    { "sSoundDoneMsg.data3"            , TWMessageTools::access_msg_data3            },
    { "sSoundDoneMsg.coordinates"      , TWMessageTools::access_sounddone_coords     },
    { "sSoundDoneMsg.targetObject"     , TWMessageTools::access_sounddone_target     },
    { "sSoundDoneMsg.name"             , TWMessageTools::access_sounddone_name       },

    { "sStimMsg.from"                  , TWMessageTools::access_msg_from             },
    { "sStimMsg.to"                    , TWMessageTools::access_msg_to               },
    { "sStimMsg.message"               , TWMessageTools::access_msg_message          },
    { "sStimMsg.time"                  , TWMessageTools::access_msg_time             },
    { "sStimMsg.flags"                 , TWMessageTools::access_msg_flags            },
    { "sStimMsg.data"                  , TWMessageTools::access_msg_data             },
    { "sStimMsg.data2"                 , TWMessageTools::access_msg_data2            },
    { "sStimMsg.data3"                 , TWMessageTools::access_msg_data3            },
    { "sStimMsg.stimulus"              , TWMessageTools::access_stim_stimulus        },
    { "sStimMsg.intensity"             , TWMessageTools::access_stim_intensity       },
    { "sStimMsg.sensor"                , TWMessageTools::access_stim_sensor          },
    { "sStimMsg.source"                , TWMessageTools::access_stim_source          },

    { "sScrTimerMsg.from"              , TWMessageTools::access_msg_from             },
    { "sScrTimerMsg.to"                , TWMessageTools::access_msg_to               },
    { "sScrTimerMsg.message"           , TWMessageTools::access_msg_message          },
    { "sScrTimerMsg.time"              , TWMessageTools::access_msg_time             },
    { "sScrTimerMsg.flags"             , TWMessageTools::access_msg_flags            },
    { "sScrTimerMsg.data"              , TWMessageTools::access_msg_data             },
    { "sScrTimerMsg.data2"             , TWMessageTools::access_msg_data2            },
    { "sScrTimerMsg.data3"             , TWMessageTools::access_msg_data3            },
    { "sScrTimerMsg.name"              , TWMessageTools::access_timer_name           },

    { "sTweqMsg.from"                  , TWMessageTools::access_msg_from             },
    { "sTweqMsg.to"                    , TWMessageTools::access_msg_to               },
    { "sTweqMsg.message"               , TWMessageTools::access_msg_message          },
    { "sTweqMsg.time"                  , TWMessageTools::access_msg_time             },
    { "sTweqMsg.flags"                 , TWMessageTools::access_msg_flags            },
    { "sTweqMsg.data"                  , TWMessageTools::access_msg_data             },
    { "sTweqMsg.data2"                 , TWMessageTools::access_msg_data2            },
    { "sTweqMsg.data3"                 , TWMessageTools::access_msg_data3            },
    { "sTweqMsg.Type"                  , TWMessageTools::access_tweq_type            },
    { "sTweqMsg.Op"                    , TWMessageTools::access_tweq_op              },
    { "sTweqMsg.Dir"                   , TWMessageTools::access_tweq_dir             },

    { "sWaypointMsg.from"              , TWMessageTools::access_msg_from             },
    { "sWaypointMsg.to"                , TWMessageTools::access_msg_to               },
    { "sWaypointMsg.message"           , TWMessageTools::access_msg_message          },
    { "sWaypointMsg.time"              , TWMessageTools::access_msg_time             },
    { "sWaypointMsg.flags"             , TWMessageTools::access_msg_flags            },
    { "sWaypointMsg.data"              , TWMessageTools::access_msg_data             },
    { "sWaypointMsg.data2"             , TWMessageTools::access_msg_data2            },
    { "sWaypointMsg.data3"             , TWMessageTools::access_msg_data3            },
    { "sWaypointMsg.moving_terrain"    , TWMessageTools::access_waypoint_mterr       },

    { "sMovingTerrainMsg.from"         , TWMessageTools::access_msg_from             },
    { "sMovingTerrainMsg.to"           , TWMessageTools::access_msg_to               },
    { "sMovingTerrainMsg.message"      , TWMessageTools::access_msg_message          },
    { "sMovingTerrainMsg.time"         , TWMessageTools::access_msg_time             },
    { "sMovingTerrainMsg.flags"        , TWMessageTools::access_msg_flags            },
    { "sMovingTerrainMsg.data"         , TWMessageTools::access_msg_data             },
    { "sMovingTerrainMsg.data2"        , TWMessageTools::access_msg_data2            },
    { "sMovingTerrainMsg.data3"        , TWMessageTools::access_msg_data3            },
    { "sMovingTerrainMsg.waypoint"     , TWMessageTools::access_movingterr_waypoint  },

    { "sQuestMsg.from"                 , TWMessageTools::access_msg_from             },
    { "sQuestMsg.to"                   , TWMessageTools::access_msg_to               },
    { "sQuestMsg.message"              , TWMessageTools::access_msg_message          },
    { "sQuestMsg.time"                 , TWMessageTools::access_msg_time             },
    { "sQuestMsg.flags"                , TWMessageTools::access_msg_flags            },
    { "sQuestMsg.data"                 , TWMessageTools::access_msg_data             },
    { "sQuestMsg.data2"                , TWMessageTools::access_msg_data2            },
    { "sQuestMsg.data3"                , TWMessageTools::access_msg_data3            },
    { "sQuestMsg.m_pName"              , TWMessageTools::access_quest_name           },
    { "sQuestMsg.m_oldValue"           , TWMessageTools::access_quest_oldvalue       },
    { "sQuestMsg.m_newValue"           , TWMessageTools::access_quest_newvalue       },

    { "sMediumTransMsg.from"           , TWMessageTools::access_msg_from             },
    { "sMediumTransMsg.to"             , TWMessageTools::access_msg_to               },
    { "sMediumTransMsg.message"        , TWMessageTools::access_msg_message          },
    { "sMediumTransMsg.time"           , TWMessageTools::access_msg_time             },
    { "sMediumTransMsg.flags"          , TWMessageTools::access_msg_flags            },
    { "sMediumTransMsg.data"           , TWMessageTools::access_msg_data             },
    { "sMediumTransMsg.data2"          , TWMessageTools::access_msg_data2            },
    { "sMediumTransMsg.data3"          , TWMessageTools::access_msg_data3            },
    { "sMediumTransMsg.nFromType"      , TWMessageTools::access_mediumtrans_from     },
    { "sMediumTransMsg.nToType"        , TWMessageTools::access_mediumtrans_to       },

    { "sYorNMsg.from"                  , TWMessageTools::access_msg_from             },
    { "sYorNMsg.to"                    , TWMessageTools::access_msg_to               },
    { "sYorNMsg.message"               , TWMessageTools::access_msg_message          },
    { "sYorNMsg.time"                  , TWMessageTools::access_msg_time             },
    { "sYorNMsg.flags"                 , TWMessageTools::access_msg_flags            },
    { "sYorNMsg.data"                  , TWMessageTools::access_msg_data             },
    { "sYorNMsg.data2"                 , TWMessageTools::access_msg_data2            },
    { "sYorNMsg.data3"                 , TWMessageTools::access_msg_data3            },
    { "sYorNMsg.YorN"                  , TWMessageTools::access_yorno_yorn           },

    { "sKeypadMsg.from"                , TWMessageTools::access_msg_from             },
    { "sKeypadMsg.to"                  , TWMessageTools::access_msg_to               },
    { "sKeypadMsg.message"             , TWMessageTools::access_msg_message          },
    { "sKeypadMsg.time"                , TWMessageTools::access_msg_time             },
    { "sKeypadMsg.flags"               , TWMessageTools::access_msg_flags            },
    { "sKeypadMsg.data"                , TWMessageTools::access_msg_data             },
    { "sKeypadMsg.data2"               , TWMessageTools::access_msg_data2            },
    { "sKeypadMsg.data3"               , TWMessageTools::access_msg_data3            },
    { "sKeypadMsg.code"                , TWMessageTools::access_keypad_code          },
};
