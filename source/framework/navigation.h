#include "../common/common.h"

void initializeInputs();
void updateInputs();

bool vpadGetTouchInfo(s32* screenX, s32* screenY);
void vpadUpdateSWKBD();
Vector2f getLeftStick();

struct ButtonState
{
    struct ButtonInfo
    {
        bool isDown;
        bool changedState;
    };

    ButtonInfo buttonA;
    ButtonInfo buttonB;
    ButtonInfo buttonStart;
};

ButtonState& GetButtonState();
bool navigatedUp();
bool navigatedDown();
bool pressedStart();
bool pressedOk();