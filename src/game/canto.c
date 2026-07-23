#include "canto.h"

static unsigned char envido_step_value(unsigned char envidos, unsigned char real)
{
    return (unsigned char)(2 * envidos + 3 * real);
}

unsigned char envido_quiero_value(const EnvidoChain *chain, unsigned char falta_target)
{
    if (chain->falta_envido) return falta_target;
    return envido_step_value(chain->envidos, chain->real_envido);
}

unsigned char envido_no_quiero_value(const EnvidoChain *before)
{
    unsigned char v = envido_step_value(before->envidos, before->real_envido);
    return v == 0 ? 1 : v;
}

unsigned char envido_can_sing_envido(const EnvidoChain *chain)
{
    return (unsigned char)(!chain->falta_envido && !chain->real_envido && chain->envidos < 2);
}

unsigned char envido_can_sing_real(const EnvidoChain *chain)
{
    return (unsigned char)(!chain->falta_envido && !chain->real_envido);
}

unsigned char envido_can_sing_falta(const EnvidoChain *chain)
{
    return (unsigned char)(!chain->falta_envido);
}

static unsigned char flor_step_value(unsigned char step)
{
    if (step == FLOR_STEP_FLOR) return 3;
    if (step == FLOR_STEP_CONTRAFLOR) return 6;
    return 0; /* FLOR_STEP_NONE */
}

unsigned char flor_quiero_value(const FlorChain *chain, unsigned char falta_target)
{
    if (chain->step == FLOR_STEP_CONTRAFLOR_RESTO) return falta_target;
    return flor_step_value(chain->step);
}

unsigned char flor_no_quiero_value(const FlorChain *before)
{
    unsigned char v = flor_step_value(before->step);
    return v == 0 ? 1 : v;
}

unsigned char flor_can_sing_contraflor(const FlorChain *chain)
{
    return (unsigned char)(chain->step == FLOR_STEP_FLOR);
}

unsigned char flor_can_sing_contraflor_resto(const FlorChain *chain)
{
    return (unsigned char)(chain->step == FLOR_STEP_CONTRAFLOR);
}

unsigned char truco_hand_value(const TrucoChain *chain)
{
    return (unsigned char)(chain->step + 1);
}

unsigned char truco_quiero_value(unsigned char new_step)
{
    return (unsigned char)(new_step + 1);
}

unsigned char truco_no_quiero_value(const TrucoChain *before)
{
    return (unsigned char)(before->step + 1);
}

unsigned char truco_can_sing_truco(const TrucoChain *chain)
{
    return (unsigned char)(chain->step == TRUCO_STEP_NONE);
}

unsigned char truco_can_sing_retruco(const TrucoChain *chain)
{
    return (unsigned char)(chain->step == TRUCO_STEP_TRUCO);
}

unsigned char truco_can_sing_vale_cuatro(const TrucoChain *chain)
{
    return (unsigned char)(chain->step == TRUCO_STEP_RETRUCO);
}
