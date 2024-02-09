//
// Created by Miktad Cakmak on 1/26/24.
//

#ifndef BMS_FSM_FSM_H
#define BMS_FSM_FSM_H

typedef enum{
    DISABLED, ENABLED //Add states (discuss with Silas/Link
}FSM_STATE;

typedef struct {
    FSM_STATE presentState;
};

#endif //BMS_FSM_FSM_H
