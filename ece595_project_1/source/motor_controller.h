
typedef enum
{
    CW = 0,
    CCW
} Direction_T;


class Motor
{
    private:
    Direction_T last_dir = CW;
    bool is_asleep;
    float rel_position = 0; /* CW rotation is positive */

    void Set_Direction(Direction_T new_dir);

    public:
    void Enable_Driver(void);
    void Disable_Driver(void);
    void Sleep(void);
    void Wakeup(void);
    void Reset_Driver(void);
    void Rotate(float degrees);
    void Zero_Position(void);
};
