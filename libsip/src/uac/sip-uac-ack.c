#include "sip-uac.h"
#include "sip-dialog.h"
#include "sip-message.h"
#include "sip-uac-transaction.h"
#include <stdio.h>

// 17.1.1.3 Construction of the ACK Request(Section 13.) (p129)
int sip_uac_ack(struct sip_uac_transaction_t* t, struct sip_dialog_t* dialog, int newtransaction)
{
	int r;
	char ptr[1024];
	char contact[1024];
	struct sip_via_t* via;
	struct sip_message_t* req;

	r = 0;
	req = sip_message_create(SIP_MESSAGE_REQUEST);
	if (0 != sip_message_init2(req, SIP_METHOD_ACK, dialog))
	{
		sip_message_destroy(req);
		return -1;
	}

	// 8.1.1.7 Via (p39)
	// The branch parameter value MUST be unique across space and time for
	// all requests sent by the UA.The exceptions to this rule are CANCEL
	// and ACK for non-2xx responses.As discussed below, a CANCEL request
	// will have the same value of the branch parameter as the request it
	// cancels.As discussed in Section 17.1.1.3, an ACK for a non-2xx
	// response will also have the same branch ID as the INVITE whose
	// response it acknowledges.

	// 13.2.2.4 2xx Responses (p82)
	// The UAC core MUST generate an ACK request for each 2xx received from
	// the transaction layer.The header fields of the ACK are constructed
	// in the same way as for any request sent within a dialog(see Section
	// 12) with the exception of the CSeq and the header fields related to
	// authentication.The sequence number of the CSeq header field MUST be
	// the same as the INVITE being acknowledged, but the CSeq method MUST
	// be ACK.

	// 17 Transactions (p122)
	// In the case of a transaction where the request was an INVITE(known as an 
	// INVITE transaction), the transaction also includes the ACK only if the 
	// final response was not a 2xx response.If the response was a 2xx, 
	// the ACK is not considered part of the transaction.

	// 6 Definitions: SIP Transaction (p24)
	// 1. If the request is INVITE and the final response is a non-2xx, the transaction also
	//	  includes an ACK to the response. 
	// 2. The ACK for a 2xx response to an INVITE request is a separate transaction(new branch value).
	if (newtransaction)
	{
		// https://www.ietf.org/mail-archive/web/sip/current/msg06460.html
		// [Sip] Branch in INVITE ,ACK,BYE
		r = sip_uac_transaction_via(t, ptr, sizeof(ptr), contact, sizeof(contact));
		if(0 == r)
			r = sip_message_add_header(req, "Via", ptr);
	}
	else
	{
		// rfc3263 4-Client Usage (p5)
		// once a SIP server has successfully been contacted (success is defined below), 
		// all retransmissions of the SIP request and the ACK for non-2xx SIP responses 
		// to INVITE MUST be sent to the same host.
		// Furthermore, a CANCEL for a particular SIP request MUST be sent to the same 
		// SIP server that the SIP request was delivered to.

		// The ACK MUST contain a single Via header field, and this MUST 
		// be equal to the top Via header field of the original request.
		via = sip_vias_get(&t->req->vias, 0);
		if (via)
			r = sip_vias_push(&req->vias, via);
	}

	// message
	t->size = sip_message_write(req, t->data, sizeof(t->data));
	// destroy sip message
	sip_message_destroy(req);

	if (0 != r || t->size <= 0 || t->size >= sizeof(t->data))
		return 0 == r ? -1 : r; // E2BIG
	return t->transport.send(t->transportptr, t->data, t->size);
}
