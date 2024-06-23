/**
 * @file states.h
 * @brief Framework for the state machine.
 * 
 * Loosly based on code from the burgler alarm extension of this project:
 * https://github.com/jgOhYeah/BikeHorn
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-23
 */
struct StatesList;

/**
 * @brief Class for a state in a state machine
 * 
 */
class State {
    public:
        /**
         * @brief Runs the state.
         * 
         * @return State* the next state to run. To break out of the state
         *         machine, return nullptr.
         */
        virtual State* enter() {}
        const char* name;
        static StatesList states;

    protected:
        State(const char* name) : name(name) {}
};

struct StatesList {
    // StateInit* init;
    // StateSleep* sleep;
    // StateAwake* awake;
    // StateAlert* alert;
    // StateCountdown* countdown;
    // StateSiren* siren;
};