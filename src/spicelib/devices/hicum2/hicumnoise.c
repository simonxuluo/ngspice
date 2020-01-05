/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1987 Gary W. Ng
Model Author: 1990 Michael Schröter TU Dresden
Spice3 Implementation: 2019 Dietmar Warning
**********/

#include "ngspice/ngspice.h"
#include "hicumdefs.h"
#include "ngspice/cktdefs.h"
#include "ngspice/iferrmsg.h"
#include "ngspice/noisedef.h"
#include "ngspice/suffix.h"

/*
 * HICUMnoise (mode, operation, firstModel, ckt, data, OnDens)
 *
 *    This routine names and evaluates all of the noise sources
 *    associated with HICUM's.  It starts with the model *firstModel and
 *    traverses all of its insts.  It then proceeds to any other models
 *    on the linked list.  The total output noise density generated by
 *    all of the HICUM's is summed with the variable "OnDens".
 */


int
HICUMnoise (int mode, int operation, GENmodel *genmodel, CKTcircuit *ckt, Ndata *data, double *OnDens)
{
    NOISEAN *job = (NOISEAN *) ckt->CKTcurJob;

    HICUMmodel *firstModel = (HICUMmodel *) genmodel;
    HICUMmodel *model;
    HICUMinstance *inst;
    double tempOnoise;
    double tempInoise;
    double noizDens[HICUMNSRCS];
    double lnNdens[HICUMNSRCS];
    int i;

    double Ibbp_Vbbp;
    double Icic_Vcic;
    double Ieie_Veie;
    double Isis_Vsis;

    /* define the names of the noise sources */

    static char *HICUMnNames[HICUMNSRCS] = {
        /* Note that we have to keep the order consistent with the
          strchr definitions in HICUMdefs.h */
        "_rc",              /* noise due to rc */
        "_rb",              /* noise due to rb */
        "_rbi",             /* noise due to rbi */
        "_re",              /* noise due to re */
        "_rs",              /* noise due to rs */
        "_ic",              /* noise due to ic */
        "_ibc",             /* noise due to ib */
        "_ibep",            /* noise due to ibep */
        "_its",             /* noise due to iccp */
        "_1overfbe",        /* flicker (1/f) noise ibe */
        "_1overfbep",       /* flicker (1/f) noise re */
        ""                  /* total transistor noise */
    };

    for (model=firstModel; model != NULL; model=HICUMnextModel(model)) {
        for (inst=HICUMinstances(model); inst != NULL;
                inst=HICUMnextInstance(inst)) {

            Ibbp_Vbbp    = 1/inst->HICUMrbx_t;
            Icic_Vcic    = 1/inst->HICUMrcx_t;
            Ieie_Veie    = 1/inst->HICUMre_t;
            Isis_Vsis    = 1/model->HICUMrsu;

            switch (operation) {

            case N_OPEN:

                /* see if we have to to produce a summary report */
                /* if so, name all the noise generators */

                if (job->NStpsSm != 0) {
                    switch (mode) {

                    case N_DENS:
                        for (i=0; i < HICUMNSRCS; i++) {
                            NOISE_ADD_OUTVAR(ckt, data, "onoise_%s%s", inst->HICUMname, HICUMnNames[i]);
                        }
                        break;

                    case INT_NOIZ:
                        for (i=0; i < HICUMNSRCS; i++) {
                            NOISE_ADD_OUTVAR(ckt, data, "onoise_total_%s%s", inst->HICUMname, HICUMnNames[i]);
                            NOISE_ADD_OUTVAR(ckt, data, "inoise_total_%s%s", inst->HICUMname, HICUMnNames[i]);
                        }
                        break;
                    }
                }
                break;

            case N_CALC:
                switch (mode) {

                case N_DENS:
                    NevalSrc(&noizDens[HICUMRCNOIZ],&lnNdens[HICUMRCNOIZ],
                                 ckt,THERMNOISE,inst->HICUMcollCINode,inst->HICUMcollNode,
                                 Icic_Vcic);

                    NevalSrc(&noizDens[HICUMRBNOIZ],&lnNdens[HICUMRBNOIZ],
                                 ckt,THERMNOISE,inst->HICUMbaseBPNode,inst->HICUMbaseNode,
                                 Ibbp_Vbbp);

                    NevalSrc(&noizDens[HICUMRBINOIZ],&lnNdens[HICUMRBINOIZ],
                                 ckt,THERMNOISE,inst->HICUMbaseBPNode,inst->HICUMbaseBINode,
                                 *(ckt->CKTstate0 + inst->HICUMibpbi_Vbpbi));

                    NevalSrc(&noizDens[HICUMRENOIZ],&lnNdens[HICUMRENOIZ],
                                 ckt,THERMNOISE,inst->HICUMemitEINode,inst->HICUMemitNode,
                                 Ieie_Veie);

                    NevalSrc(&noizDens[HICUMRSNOIZ],&lnNdens[HICUMRSNOIZ],
                                 ckt,THERMNOISE,inst->HICUMsubsSINode,inst->HICUMsubsNode,
                                 Isis_Vsis);


                    NevalSrc(&noizDens[HICUMICNOIZ],&lnNdens[HICUMICNOIZ],
                                 ckt,SHOTNOISE,inst->HICUMcollCINode,inst->HICUMemitEINode,
                                 *(ckt->CKTstate0 + inst->HICUMiciei));

                    NevalSrc(&noizDens[HICUMIBCNOIZ],&lnNdens[HICUMIBCNOIZ],
                                 ckt,SHOTNOISE,inst->HICUMbaseBINode,inst->HICUMcollCINode,
                                 *(ckt->CKTstate0 + inst->HICUMibici));

                    NevalSrc(&noizDens[HICUMIBEPNOIZ],&lnNdens[HICUMIBEPNOIZ],
                                 ckt,SHOTNOISE,inst->HICUMbaseBPNode,inst->HICUMemitEINode,
                                 *(ckt->CKTstate0 + inst->HICUMibpei));

                    NevalSrc(&noizDens[HICUMIBCXNOIZ],&lnNdens[HICUMIBCXNOIZ],
                                 ckt,SHOTNOISE,inst->HICUMbaseBPNode,inst->HICUMcollCINode,
                                 *(ckt->CKTstate0 + inst->HICUMibpci));

                    NevalSrc(&noizDens[HICUMITSNOIZ],&lnNdens[HICUMITSNOIZ],
                                 ckt,SHOTNOISE,inst->HICUMsubsSINode,inst->HICUMcollCINode,
                                 *(ckt->CKTstate0 + inst->HICUMisici));


                    NevalSrc(&noizDens[HICUMFLBENOIZ], NULL, ckt,
                                 N_GAIN,inst->HICUMbaseBINode, inst->HICUMemitEINode,
                                 (double)0.0);
                    noizDens[HICUMFLBENOIZ] *= inst->HICUMm * model->HICUMkf * 
                                 exp(model->HICUMaf *
                                 log(MAX(fabs((*(ckt->CKTstate0 + inst->HICUMibiei)+*(ckt->CKTstate0 + inst->HICUMibpei))/inst->HICUMm),N_MINLOG))) /
                                 data->freq;
                    lnNdens[HICUMFLBENOIZ] = 
                                 log(MAX(noizDens[HICUMFLBENOIZ],N_MINLOG));

                    NevalSrc(&noizDens[HICUMFLRENOIZ], NULL, ckt,
                                 N_GAIN,inst->HICUMemitNode, inst->HICUMemitEINode,
                                 (double)0.0);
                    noizDens[HICUMFLRENOIZ] *= inst->HICUMm * model->HICUMkfre * 
                                 exp(model->HICUMafre *
                                 log(MAX(fabs(*(ckt->CKTstate0 + inst->HICUMieie)/inst->HICUMm),N_MINLOG))) /
                                 data->freq;
                    lnNdens[HICUMFLRENOIZ] = 
                                 log(MAX(noizDens[HICUMFLRENOIZ],N_MINLOG));


                    noizDens[HICUMTOTNOIZ] = noizDens[HICUMRCNOIZ]   +
                                             noizDens[HICUMRBNOIZ]   +
                                             noizDens[HICUMRBINOIZ]  +
                                             noizDens[HICUMRENOIZ]   +
                                             noizDens[HICUMRSNOIZ]   +
                                             noizDens[HICUMICNOIZ]   +
                                             noizDens[HICUMIBCNOIZ]  +
                                             noizDens[HICUMIBEPNOIZ] +
                                             noizDens[HICUMIBCXNOIZ] +
                                             noizDens[HICUMITSNOIZ]  +
                                             noizDens[HICUMFLBENOIZ] +
                                             noizDens[HICUMFLRENOIZ];


                    lnNdens[HICUMTOTNOIZ] = 
                                 log(noizDens[HICUMTOTNOIZ]);

                    *OnDens += noizDens[HICUMTOTNOIZ];

                    if (data->delFreq == 0.0) { 

                        /* if we haven't done any previous integration, we need to */
                        /* initialize our "history" variables                      */

                        for (i=0; i < HICUMNSRCS; i++) {
                            inst->HICUMnVar[LNLSTDENS][i] = lnNdens[i];
                        }

                        /* clear out our integration variables if it's the first pass */

                        if (data->freq == job->NstartFreq) {
                            for (i=0; i < HICUMNSRCS; i++) {
                                inst->HICUMnVar[OUTNOIZ][i] = 0.0;
                                inst->HICUMnVar[INNOIZ][i] = 0.0;
                            }
                        }
                    } else {   /* data->delFreq != 0.0 (we have to integrate) */

/* In order to get the best curve fit, we have to integrate each component separately */

                        for (i=0; i < HICUMNSRCS; i++) {
                            if (i != HICUMTOTNOIZ) {
                                tempOnoise = Nintegrate(noizDens[i], lnNdens[i],
                                      inst->HICUMnVar[LNLSTDENS][i], data);
                                tempInoise = Nintegrate(noizDens[i] * data->GainSqInv ,
                                      lnNdens[i] + data->lnGainInv,
                                      inst->HICUMnVar[LNLSTDENS][i] + data->lnGainInv,
                                      data);
                                inst->HICUMnVar[LNLSTDENS][i] = lnNdens[i];
                                data->outNoiz += tempOnoise;
                                data->inNoise += tempInoise;
                                if (job->NStpsSm != 0) {
                                    inst->HICUMnVar[OUTNOIZ][i] += tempOnoise;
                                    inst->HICUMnVar[OUTNOIZ][HICUMTOTNOIZ] += tempOnoise;
                                    inst->HICUMnVar[INNOIZ][i] += tempInoise;
                                    inst->HICUMnVar[INNOIZ][HICUMTOTNOIZ] += tempInoise;
                                }
                            }
                        }
                    }
                    if (data->prtSummary) {
                        for (i=0; i < HICUMNSRCS; i++) {     /* print a summary report */
                            data->outpVector[data->outNumber++] = noizDens[i];
                        }
                    }
                    break;

                case INT_NOIZ:        /* already calculated, just output */
                    if (job->NStpsSm != 0) {
                        for (i=0; i < HICUMNSRCS; i++) {
                            data->outpVector[data->outNumber++] = inst->HICUMnVar[OUTNOIZ][i];
                            data->outpVector[data->outNumber++] = inst->HICUMnVar[INNOIZ][i];
                        }
                    }    /* if */
                    break;
                }    /* switch (mode) */
                break;

            case N_CLOSE:
                return (OK);         /* do nothing, the main calling routine will close */
                break;               /* the plots */
            }    /* switch (operation) */
        }    /* for inst */
    }    /* for model */

return(OK);
}
