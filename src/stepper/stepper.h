#define SLEEP_PIN 13
#define STEP_PIN 14
#define DIRECTION_PIN 15


void stepper_init(void);
void wake_stepper();
void sleep_stepper();
int single_step(uint direction);
int step_to(uint *current_position, uint desired_position, uint BOUNDARY_HIGH);
