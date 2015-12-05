#define IR_FREQ 38000
#define IR_PIN 4

#define AC_CMD "000000000000000000000000001001001110000000000000000000000000000000110000"

void ir_send();

void ac_init();
void ac_get_settings(char *settings, int temp_humid);
void ac_set_power(char *power);
void ac_set_mode(char *mode);
void ac_set_temp(char *temp);
void ac_set_fan(char *fan);
void ac_set_swing(char *swing);
