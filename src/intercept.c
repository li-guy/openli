/*
 *
 * Copyright (c) 2018 The University of Waikato, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This file is part of OpenLI.
 *
 * This code has been developed by the University of Waikato WAND
 * research group. For further information please see http://www.wand.net.nz/
 *
 * OpenLI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenLI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#include "intercept.h"
static inline void copy_intercept_common(intercept_common_t *src,
        intercept_common_t *dest) {

    dest->liid = strdup(src->liid);
    dest->authcc = strdup(src->authcc);
    dest->delivcc = strdup(src->delivcc);

    if (src->targetagency) {
        dest->targetagency = strdup(src->targetagency);
    } else {
        dest->targetagency = NULL;
    }

    dest->liid_len = src->liid_len;
    dest->authcc_len = src->authcc_len;
    dest->delivcc_len = src->delivcc_len;
    dest->destid = src->destid;
}


rtpstreaminf_t *create_rtpstream(voipintercept_t *vint, uint32_t cin) {

    rtpstreaminf_t *newcin = NULL;

    newcin = (rtpstreaminf_t *)malloc(sizeof(rtpstreaminf_t));
    if (!newcin) {
        return NULL;
    }

    newcin->streamkey = (char *)calloc(1, 256);
    if (!newcin->streamkey) {
        free(newcin);
        return NULL;
    }
    newcin->cin = cin;
    newcin->parent = vint;
    newcin->active = 0;
    newcin->targetaddr = NULL;
    newcin->otheraddr = NULL;
    newcin->ai_family = 0;
    newcin->targetport = 0;
    newcin->otherport = 0;
    newcin->seqno = 0;
    newcin->invitecseq = NULL;
    newcin->byecseq = NULL;
    newcin->timeout_ev = NULL;
    newcin->byematched = 0;

    copy_intercept_common(&(vint->common), &(newcin->common));
    snprintf(newcin->streamkey, 256, "%s-%u", vint->common.liid, cin);
    return newcin;
}

rtpstreaminf_t *deep_copy_rtpstream(rtpstreaminf_t *orig) {
    rtpstreaminf_t *copy = NULL;

    copy = (rtpstreaminf_t *)malloc(sizeof(rtpstreaminf_t));
    if (!copy) {
        return NULL;
    }

    copy->streamkey = strdup(orig->streamkey);
    copy->cin = orig->cin;
    copy->parent = NULL;
    copy->ai_family = orig->ai_family;
    copy->targetaddr = (struct sockaddr_storage *)malloc(
            sizeof(struct sockaddr_storage));

    if (!copy->targetaddr) {
        free(copy);
        return NULL;
    }

    memcpy(copy->targetaddr, orig->targetaddr, sizeof(struct sockaddr_storage));

    copy->otheraddr = (struct sockaddr_storage *)malloc(
            sizeof(struct sockaddr_storage));
    if (!copy->otheraddr) {
        free(copy->targetaddr);
        free(copy);
        return NULL;
    }

    memcpy(copy->otheraddr, orig->otheraddr, sizeof(struct sockaddr_storage));
    copy->targetport = orig->targetport;
    copy->otherport = orig->otherport;
    copy->seqno = 0;
    copy->active = 1;
    copy->invitecseq = NULL;
    copy->byecseq = NULL;
    copy->timeout_ev = NULL;
    copy_intercept_common(&(orig->common), &(copy->common));

    return copy;
}


static inline void free_intercept_common(intercept_common_t *cept) {

    if (cept->liid) {
        free(cept->liid);
    }

    if (cept->authcc) {
        free(cept->authcc);
    }

    if (cept->delivcc) {
        free(cept->delivcc);
    }

    if (cept->targetagency) {
        free(cept->targetagency);
    }
}

void free_single_ipintercept(ipintercept_t *cept) {
    free_intercept_common(&(cept->common));
    if (cept->username) {
        free(cept->username);
    }

    free(cept);
}

void free_all_ipintercepts(ipintercept_t *interceptlist) {

    ipintercept_t *cept, *tmp;

    HASH_ITER(hh_liid, interceptlist, cept, tmp) {
        HASH_DELETE(hh_liid, interceptlist, cept);
        free_single_ipintercept(cept);
    }
}

static inline void free_voip_cinmap(voipcinmap_t *cins) {
    voipcinmap_t *c, *tmp;

    HASH_ITER(hh_callid, cins, c, tmp) {
        HASH_DELETE(hh_callid, cins, c);
        free(c->shared);
        free(c->callid);
        free(c);
    }

}

static inline void free_voip_sdpmap(voipsdpmap_t *sdps) {
    voipsdpmap_t *s, *tmp;

    HASH_ITER(hh_sdp, sdps, s, tmp) {
        HASH_DELETE(hh_sdp, sdps, s);
        free(s);
    }
}

void free_single_voip_cin(rtpstreaminf_t *rtp) {
    free_intercept_common(&(rtp->common));
    if (rtp->invitecseq) {
        free(rtp->invitecseq);
    }
    if (rtp->byecseq) {
        free(rtp->byecseq);
    }
    if (rtp->targetaddr) {
        free(rtp->targetaddr);
    }
    if (rtp->otheraddr) {
        free(rtp->otheraddr);
    }
    if (rtp->streamkey) {
        free(rtp->streamkey);
    }
    if (rtp->timeout_ev) {
        free(rtp->timeout_ev);
    }
    free(rtp);
}

static void free_voip_cins(rtpstreaminf_t *cins) {
    rtpstreaminf_t *rtp, *tmp;

    HASH_ITER(hh, cins, rtp, tmp) {
        HASH_DEL(cins, rtp);
        free_single_voip_cin(rtp);
    }

}

void free_single_voipintercept(voipintercept_t *v) {
    free_intercept_common(&(v->common));
    if (v->sipuri) {
        free(v->sipuri);
    }
    if (v->cin_sdp_map) {
        free_voip_sdpmap(v->cin_sdp_map);
    }

    if (v->cin_callid_map) {
        free_voip_cinmap(v->cin_callid_map);
    }
    if (v->active_cins) {
        free_voip_cins(v->active_cins);
    }
    free(v);
}

void free_all_voipintercepts(voipintercept_t *vints) {

    voipintercept_t *v, *tmp;
    HASH_ITER(hh_liid, vints, v, tmp) {
        HASH_DELETE(hh_liid, vints, v);
        free_single_voipintercept(v);
    }

}

void free_all_rtpstreams(rtpstreaminf_t *streams) {
    rtpstreaminf_t *rtp, *tmp;

    HASH_ITER(hh, streams, rtp, tmp) {
        HASH_DELETE(hh, streams, rtp);
        free_intercept_common(&(rtp->common));
        if (rtp->targetaddr) {
            free(rtp->targetaddr);
        }
        if (rtp->otheraddr) {
            free(rtp->otheraddr);
        }
        if (rtp->streamkey) {
            free(rtp->streamkey);
        }
        free(rtp);
    }
}

ipsession_t *create_ipsession(ipintercept_t *ipint, access_session_t *session) {

    ipsession_t *ipsess;

    ipsess = (ipsession_t *)malloc(sizeof(ipsession_t));
    if (ipsess == NULL) {
        return NULL;
    }

    ipsess->cin = session->cin;
    ipsess->ai_family = session->ipfamily;
    ipsess->targetip = (struct sockaddr_storage *)(malloc(
            sizeof(struct sockaddr_storage)));
    if (!ipsess->targetip) {
        free(ipsess);
        return NULL;
    }
    memcpy(ipsess->targetip, session->assignedip,
            sizeof(struct sockaddr_storage));

    copy_intercept_common(&(ipint->common), &(ipsess->common));

    ipsess->streamkey = (char *)(calloc(1, 256));
    if (!ipsess->streamkey) {
        free(ipsess->targetip);
        free(ipsess);
        return NULL;
    }
    snprintf(ipsess->streamkey, 256, "%s-%u", ipint->common.liid, session->cin);
    return ipsess;
}

void free_single_ipsession(ipsession_t *sess) {

    if (sess->streamkey) {
        free(sess->streamkey);
    }
    if (sess->targetip) {
        free(sess->targetip);
    }
    free(sess);
}

void free_all_ipsessions(ipsession_t *sessions) {
    ipsession_t *s, *tmp;
    HASH_ITER(hh, sessions, s, tmp) {
        HASH_DELETE(hh, sessions, s);
        free_single_ipsession(s);
    }
}
// vim: set sw=4 tabstop=4 softtabstop=4 expandtab :
