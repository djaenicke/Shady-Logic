typedef enum
{
    POSITION_NOT_LEARNED=0,
    REQUEST_CLOSED_POSITION,
    SEEK_OPEN_POSITION,
    POSITION_LEARNED,
    MANUAL_CONTROL,
    LIGHT_CONTROL,
    TIME_CONTROL,
    IDLE
} Control_State_T;

extern void Init_Blinds_Control(void);
extern void Ctrl_State_Machine(void);
extern void Toggle_Blinds_State(void);
extern void Change_Control_State(Control_State_T new_state);
