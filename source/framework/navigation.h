#include "../common/common.h"

void initializeInputs();
void updateInputs();

void vpadGetTouchInfo(bool& isTouchValid, s32& screenX, s32& screenY);
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
bool pressedStart();