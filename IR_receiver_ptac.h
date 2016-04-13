#ifndef __IR_RECEIVER_PTAC_H_
#define __IR_RECEIVER_PTAC_H_

void onCodeFinish();
void matchCode();
uint16_t applyResolution(uint16_t val);
void listenIR(void);

#endif