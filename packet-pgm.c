/* packet-pgm.c
 * Routines for pgm packet disassembly
 *
 * $Id: packet-pgm.c,v 1.2 2001/07/12 21:48:46 guy Exp $
 * 
 * Copyright (c) 2000 by Talarian Corp
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1999 Gerald Combs
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include "packet.h"
#include "packet-pgm.h"
#if 0
#include "resolv.h"
#include "prefs.h"
#include "strutil.h"
#endif 

#include "proto.h"
#ifndef IP_PROTO_PGM
#define IP_PROTO_PGM 113
#endif

void proto_reg_handoff_pgm(void);
extern void decode_tcp_ports(tvbuff_t *, int , packet_info *,
    proto_tree *, int, int);


static int proto_pgm = -1;
static int ett_pgm = -1;
static int ett_pgm_optbits = -1;
static int ett_pgm_opts = -1;
static int ett_pgm_spm = -1;
static int ett_pgm_data = -1;
static int ett_pgm_nak = -1;
static int ett_pgm_opts_join = -1;
static int ett_pgm_opts_parityprm = -1;
static int ett_pgm_opts_paritygrp = -1;
static int ett_pgm_opts_naklist = -1;

static int hf_pgm_main_sport = -1;
static int hf_pgm_main_dport = -1;
static int hf_pgm_main_type = -1;
static int hf_pgm_main_opts = -1;
static int hf_pgm_main_opts_opt = -1;
static int hf_pgm_main_opts_netsig = -1;
static int hf_pgm_main_opts_varlen = -1;
static int hf_pgm_main_opts_parity = -1;
static int hf_pgm_main_cksum = -1;
static int hf_pgm_main_gsi = -1;
static int hf_pgm_main_tsdulen = -1;
static int hf_pgm_spm_sqn = -1;
static int hf_pgm_spm_lead = -1;
static int hf_pgm_spm_trail = -1;
static int hf_pgm_spm_pathafi = -1;
static int hf_pgm_spm_res = -1;
static int hf_pgm_spm_path = -1;
static int hf_pgm_data_sqn = -1;
static int hf_pgm_data_trail = -1;
static int hf_pgm_nak_sqn = -1;
static int hf_pgm_nak_srcafi = -1;
static int hf_pgm_nak_srcres = -1;
static int hf_pgm_nak_src = -1;
static int hf_pgm_nak_grpafi = -1;
static int hf_pgm_nak_grpres = -1;
static int hf_pgm_nak_grp = -1;

static int hf_pgm_opt_type = -1;
static int hf_pgm_opt_len = -1;
static int hf_pgm_opt_tlen = -1;

static int hf_pgm_genopt_type = -1;
static int hf_pgm_genopt_len = -1;
static int hf_pgm_genopt_opx = -1;
static int hf_pgm_genopt_res = -1;

static int hf_pgm_opt_join_type = -1;
static int hf_pgm_opt_join_len = -1;
static int hf_pgm_opt_join_opx = -1;
static int hf_pgm_opt_join_res = -1;
static int hf_pgm_opt_join_minjoin = -1;

static int hf_pgm_opt_parity_prm_type = -1;
static int hf_pgm_opt_parity_prm_len = -1;
static int hf_pgm_opt_parity_prm_opx = -1;
static int hf_pgm_opt_parity_prm_po = -1;
static int hf_pgm_opt_parity_prm_prmtgsz = -1;

static int hf_pgm_opt_parity_grp_type = -1;
static int hf_pgm_opt_parity_grp_len = -1;
static int hf_pgm_opt_parity_grp_opx = -1;
static int hf_pgm_opt_parity_grp_res = -1;
static int hf_pgm_opt_parity_grp_prmgrp = -1;

static int hf_pgm_opt_curr_tgsize_type = -1;
static int hf_pgm_opt_curr_tgsize_len = -1;
static int hf_pgm_opt_curr_tgsize_opx = -1;
static int hf_pgm_opt_curr_tgsize_res = -1;
static int hf_pgm_opt_curr_tgsize_prmatgsz = -1;

static int hf_pgm_opt_nak_type = -1;
static int hf_pgm_opt_nak_len = -1;
static int hf_pgm_opt_nak_opx = -1;
static int hf_pgm_opt_nak_res = -1;
static int hf_pgm_opt_nak_list = -1;

inline char *
gsistr(char *gsi, int len)
{
	 static char msg[256];
	 char *p = msg;
	 int i;

	for (i = 0; i <= 255 && i < len; i++) {
		sprintf(p,"%02x",(nchar_t)gsi[i]);
		p += 2;
	}
	return(msg);
}
inline char *
optsstr(nchar_t opts)
{
	 static char msg[256];
	 char *p = msg, *str;

	if (opts == 0)
		return("");

	if (opts & PGM_OPT){
		sprintf(p, "Present");
		p += strlen("Present");
	}
	if (opts & PGM_OPT_NETSIG){
		if (p != msg)
			str = ",NetSig";
		else
			str = "NetSig";
		sprintf(p, str);
		p += strlen(str);
	}
	if (opts & PGM_OPT_VAR_PKTLEN){
		if (p != msg)
			str = ",VarLen";
		else
			str = "VarLen";
		sprintf(p, str);
		p += strlen(str);
	}
	if (opts & PGM_OPT_PARITY){
		if (p != msg)
			str = ",Parity";
		else
			str = "Parity";
		sprintf(p, str);
		p += strlen(str);
	}
	if (p == msg) {
		sprintf(p, "0x%x", opts);
	}
	return(msg);
}
inline char *
opttypes(nchar_t opt)
{
	 static char msg[128];

	if (opt == PGM_OPT_LENGTH)
		return("Length");
	if (opt == PGM_OPT_END)
		return("End");
	if (opt == PGM_OPT_FRAGMENT)
		return("Fragment");
	if (opt == PGM_OPT_NAK_LIST)
		return("NakList");
	if (opt == PGM_OPT_JOIN)
		return("Join");
	if (opt == PGM_OPT_REDIRECT)
		return("ReDirect");
	if (opt == PGM_OPT_SYN)
		return("Syn");
	if (opt == PGM_OPT_FIN)
		return("Fin");
	if (opt == PGM_OPT_RST)
		return("Rst");
	if (opt == PGM_OPT_PARITY_PRM)
		return("ParityPrm");
	if (opt == PGM_OPT_PARITY_GRP)
		return("ParityGrp");
	if (opt == PGM_OPT_CURR_TGSIZE)
		return("CurrTgsiz");

	sprintf(msg, "0x%x", opt);
	return(msg);
}
inline char *
opxbits(nchar_t opt)
{
	 static char msg[128];

	if (opt == 0)
		return("");

	if (opt == PGM_OPX_IGNORE)
		return("Ignore");
	if (opt == PGM_OPX_INVAL)
		return("Inval");
	if (opt == PGM_OPX_DISCARD)
		return("DisCard");

	sprintf(msg, "0x%x", opt);
	return(msg);
}
char *pktname = NULL;

static void
dissect_pgmopts(tvbuff_t *tvb, int offset, proto_tree *tree)
{
	proto_item *tf;
	proto_tree *opts_tree = NULL;
	proto_tree *opt_tree = NULL;
	pgm_opt_length_t opts;
	pgm_opt_generic_t genopts;
	int theend = 0, firsttime = 1;

	tvb_memcpy(tvb, (guint8 *)&opts, offset, sizeof(opts));
	opts.total_len = ntohs(opts.total_len);

	tf = proto_tree_add_text(tree, tvb, offset, 
		opts.total_len, 
		"%s Options (Total Length %d)", pktname, opts.total_len);
	opts_tree = proto_item_add_subtree(tf, ett_pgm_opts);
	proto_tree_add_uint_format(opts_tree, hf_pgm_opt_type, tvb, 
		offset, 4, opts.type, 
		"Option: %-9s (0x%x), Length: %d, Total Length: %d", 
		opttypes(opts.type), opts.type, opts.len, opts.total_len);

	offset += 4;
	for (opts.total_len -= 4; opts.total_len > 0;){
		tvb_memcpy(tvb, (guint8 *)&genopts, offset, sizeof(genopts));
		if (genopts.type & PGM_OPT_END)  {
			genopts.type &= ~PGM_OPT_END;
			theend = 1;
		}
		tf = proto_tree_add_uint_format(opts_tree, hf_pgm_genopt_type, tvb, 
			offset, genopts.len, genopts.type, 
			"Option: %-9s (0x%x), Length: %d", opttypes(genopts.type), 
				genopts.type, genopts.len );
		if (genopts.len == 0)
			break;

		switch(genopts.type) {
		case PGM_OPT_JOIN:{
			pgm_opt_join_t optdata;

			tvb_memcpy(tvb, (guint8 *)&optdata, offset, sizeof(optdata));
			opt_tree = proto_item_add_subtree(tf, ett_pgm_opts_join);

			proto_tree_add_uint_format(opt_tree, hf_pgm_opt_join_type, 
				tvb, offset, 1, optdata.type, "Type: %s (0x%x)", 
				opttypes(optdata.type), optdata.type);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_join_len, tvb, 
				offset+1, 1, optdata.len);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_join_opx, tvb, 
				offset+2, 1, optdata.opx);

			proto_tree_add_uint_format(opt_tree, hf_pgm_opt_join_res, tvb, 
				offset+3, 1, optdata.opx, "Reserved: 0x%x", optdata.res);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_join_minjoin, tvb, 
				offset+4, 4, ntohl(optdata.opt_join_min));

			break;
		}
		case PGM_OPT_PARITY_PRM:{
			pgm_opt_parity_prm_t optdata;

			tvb_memcpy(tvb, (guint8 *)&optdata, offset, sizeof(optdata));
			opt_tree = proto_item_add_subtree(tf, ett_pgm_opts_parityprm);

			proto_tree_add_uint_format(opt_tree, hf_pgm_opt_parity_prm_type, 
				tvb, offset, 1, optdata.type, "Type: %s (0x%x)", 
				opttypes(optdata.type), optdata.type);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_parity_prm_len, tvb, 
				offset+1, 1, optdata.len);

			proto_tree_add_uint_format(opt_tree, hf_pgm_opt_parity_prm_opx, 
				tvb, offset+2, 1, optdata.opx, 
				"Extensibility Bits: %s (0x%x)", opxbits(optdata.opx), 
				optdata.opx);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_parity_prm_po, tvb, 
				offset+3, 1, optdata.po);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_parity_prm_prmtgsz,
				tvb, offset+4, 4, ntohl(optdata.prm_tgsz));

			break;
		}
		case PGM_OPT_PARITY_GRP:{
			pgm_opt_parity_grp_t optdata;

			tvb_memcpy(tvb, (guint8 *)&optdata, offset, sizeof(optdata));
			opt_tree = proto_item_add_subtree(tf, ett_pgm_opts_paritygrp);

			proto_tree_add_uint_format(opt_tree, hf_pgm_opt_parity_prm_type, 
				tvb, offset, 1, optdata.type, "Type: %s (0x%x)", 
				opttypes(optdata.type), optdata.type);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_parity_prm_len, tvb, 
				offset+1, 1, optdata.len);

			proto_tree_add_uint_format(opt_tree, hf_pgm_opt_parity_grp_opx, 
				tvb, offset+2, 1, optdata.opx, 
				"Extensibility Bits: %s (0x%x)", opxbits(optdata.opx), 
				optdata.opx);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_parity_grp_res, tvb, 
				offset+3, 1, optdata.res);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_parity_grp_prmgrp,
				tvb, offset+4, 4, ntohl(optdata.prm_grp));

			break;
		}
		case PGM_OPT_NAK_LIST:{
			pgm_opt_nak_list_t optdata;
			nlong_t naklist[PGM_MAX_NAK_LIST_SZ+1];
			char nakbuf[8192], *ptr;
			int i, j, naks, soffset = 0;

			tvb_memcpy(tvb, (guint8 *)&optdata, offset, sizeof(optdata));
			opt_tree = proto_item_add_subtree(tf, ett_pgm_opts_naklist);

			proto_tree_add_uint_format(opt_tree, hf_pgm_opt_nak_type, tvb, 
				offset, 1, optdata.type, "Type: %s (0x%x)", 
				opttypes(optdata.type), optdata.type);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_nak_len, tvb, 
				offset+1, 1, optdata.len);

			proto_tree_add_uint_format(opt_tree, hf_pgm_opt_nak_opx, 
				tvb, offset+2, 1, optdata.opx, 
				"Extensibility Bits: %s (0x%x)", opxbits(optdata.opx), 
				optdata.opx);

			proto_tree_add_uint(opt_tree, hf_pgm_opt_nak_res, tvb, 
				offset+3, 1, optdata.res);

			optdata.len -= sizeof(pgm_opt_nak_list_t);
			tvb_memcpy(tvb, (guint8 *)naklist, offset+4, optdata.len);
			naks = (optdata.len/sizeof(nlong_t));
			ptr = nakbuf;
			j = 0;
			/*
			 * Print out 8 per line 
			 */
			for (i=0; i < naks; i++) {
				sprintf(nakbuf+soffset, "0x%lx ",
				    (unsigned long)ntohl(naklist[i]));
				soffset = strlen(nakbuf);
				if ((++j % 8) == 0) {
					if (firsttime) {
						proto_tree_add_bytes_format(opt_tree, 
							hf_pgm_opt_nak_list, tvb, offset+4, optdata.len,
							nakbuf, "List(%d): %s", naks, nakbuf);
							soffset = 0;
					} else {
						proto_tree_add_bytes_format(opt_tree, 
							hf_pgm_opt_nak_list, tvb, offset+4, optdata.len, 
							nakbuf, "List: %s", nakbuf);
							soffset = 0;
					}
					firsttime = 0;
				}
			}
			if (soffset) {
				if (firsttime) {
					proto_tree_add_bytes_format(opt_tree, 
						hf_pgm_opt_nak_list, tvb, offset+4, optdata.len,
						nakbuf, "List(%d): %s", naks, nakbuf);
						soffset = 0;
				} else {
					proto_tree_add_bytes_format(opt_tree, 
						hf_pgm_opt_nak_list, tvb, offset+4, optdata.len, 
						nakbuf, "List: %s", nakbuf);
						soffset = 0;
				}
			}
			break;
		}
		}
		offset += genopts.len;
		opts.total_len -= genopts.len;

	}
}
/*
 * dissect_pgm - The dissector for Pragmatic General Multicast
 */
static void
dissect_pgm(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_tree *pgm_tree = NULL;
	proto_tree *opt_tree = NULL;
	proto_tree *type_tree = NULL;
	proto_item *tf;
	struct hostent *hp;
	pgm_t pgmhdr;
	pgm_spm_t spm;
	pgm_data_t data;
	pgm_nak_t nak;
	int offset = 0;
	guint hlen, plen;
	proto_item *ti;
	char buf[512];
	struct in_addr inaddr;
	char tmpaddr[16];

	if (check_col(pinfo->fd, COL_PROTOCOL))
		col_set_str(pinfo->fd, COL_PROTOCOL, "PGM");

	/* Clear out the Info column. */
	if (check_col(pinfo->fd, COL_INFO))
		col_clear(pinfo->fd, COL_INFO);

	tvb_memcpy(tvb, (guint8 *)&pgmhdr, offset, sizeof(pgm_t));
	hlen = sizeof(pgm_t);
	pgmhdr.sport = ntohs(pgmhdr.sport);
	pgmhdr.dport = ntohs(pgmhdr.dport);
	pgmhdr.tsdulen = ntohs(pgmhdr.tsdulen);

	switch(pgmhdr.type) {
	case PGM_SPM_PCKT:
		plen = sizeof(pgm_spm_t);
		tvb_memcpy(tvb, (guint8 *)&spm, sizeof(pgm_t), plen);
		pktname = "SPM";
		spm_ntoh(&spm);
		inaddr.s_addr = htonl(spm.path);
		sprintf(buf, "SPM: sqn 0x%x path %s", spm.sqn, inet_ntoa(inaddr));
		break;

	case PGM_RDATA_PCKT:
		pktname = "RDATA";
		/* FALLTHUR */
	case PGM_ODATA_PCKT:
		if (pktname == NULL)
			pktname = "ODATA";
		plen = sizeof(pgm_data_t);
		tvb_memcpy(tvb, (guint8 *)&data, sizeof(pgm_t), plen);
		data_ntoh(&data);
		sprintf(buf, "%s: sqn 0x%x tsdulen %d", pktname, data.sqn, 
			pgmhdr.tsdulen);
		break;

	case PGM_NAK_PCKT:
		pktname = "NAK";
		/* FALLTHUR */
	case PGM_NNAK_PCKT:
		if (pktname == NULL)
			pktname = "NNAK";
		/* FALLTHUR */
	case PGM_NCF_PCKT:
		if (pktname == NULL)
			pktname = "NCF";
		plen = sizeof(pgm_nak_t);
		tvb_memcpy(tvb, (guint8 *)&nak, sizeof(pgm_t), plen);
		nak_ntoh(&nak);
		inaddr.s_addr = htonl(nak.src);
		strcpy(tmpaddr, inet_ntoa(inaddr));
		inaddr.s_addr = htonl(nak.grp);
		sprintf(buf, "%s: sqn 0x%x src %s grp %s", pktname, nak.sqn, 
			tmpaddr, inet_ntoa(inaddr)); 

		break;
	default:
		return;
	}

	if (check_col(pinfo->fd, COL_INFO)) {
		col_add_fstr(pinfo->fd, COL_INFO, "%s", buf);
	}
	if (tree) {
		sprintf(buf, "Pragmatic General Multicast: Type %s (%d)"
		    " SrcPort %u, DstPort %u, GSI %s", pktname,
			pgmhdr.type, pgmhdr.sport, pgmhdr.dport, gsistr(pgmhdr.gsi, 6));
		ti = proto_tree_add_protocol_format(tree, proto_pgm, 
			tvb, offset, hlen, "%s", buf);
#if 0
		ti = proto_tree_add_item(tree, proto_pgm, tvb, offset, hlen, FALSE);
#endif

		pgm_tree = proto_item_add_subtree(ti, ett_pgm);
		proto_tree_add_uint_format(pgm_tree, hf_pgm_main_sport, tvb, offset, 2,
			pgmhdr.sport, "Source port: %u", pgmhdr.sport);
		proto_tree_add_uint_format(pgm_tree, hf_pgm_main_dport, tvb, offset+2, 
			2, pgmhdr.dport, "Destination port: %u", pgmhdr.dport);
		tf = proto_tree_add_uint_format(pgm_tree, hf_pgm_main_type, tvb, 
			offset+4, 1, pgmhdr.type, "Packet type: %s (%d)", pktname, 
			pgmhdr.type);

		tf = proto_tree_add_uint_format(pgm_tree, hf_pgm_main_opts, tvb, 
			offset+5, 1, pgmhdr.opts, "Options: %s (0x%x)", 
			optsstr(pgmhdr.opts), pgmhdr.opts);
		opt_tree = proto_item_add_subtree(tf, ett_pgm_optbits);

		proto_tree_add_boolean(opt_tree, hf_pgm_main_opts_opt, tvb, 
			offset+5, 1, (pgmhdr.opts & PGM_OPT));
		proto_tree_add_boolean(opt_tree, hf_pgm_main_opts_netsig, tvb, 
			offset+5, 1, (pgmhdr.opts & PGM_OPT_NETSIG));
		proto_tree_add_boolean(opt_tree, hf_pgm_main_opts_varlen, tvb, 
			offset+5, 1, (pgmhdr.opts & PGM_OPT_VAR_PKTLEN));
		proto_tree_add_boolean(opt_tree, hf_pgm_main_opts_parity, tvb, 
			offset+5, 1, (pgmhdr.opts & PGM_OPT_PARITY));

		proto_tree_add_uint_format(pgm_tree, hf_pgm_main_cksum, tvb, offset+6, 
			2, pgmhdr.cksum, "Check Sum: 0x%x", pgmhdr.cksum);
		proto_tree_add_bytes(pgm_tree, hf_pgm_main_gsi, tvb, offset+8, 
			6, pgmhdr.gsi);
		proto_tree_add_uint_format(pgm_tree, hf_pgm_main_tsdulen, tvb, 
			offset+14, 2, pgmhdr.tsdulen, "Transport Service Data Unit: %u", 
			pgmhdr.tsdulen);

		offset = sizeof(pgm_t);
		tf = proto_tree_add_text(pgm_tree, tvb, offset, plen, "%s Packet",
			pktname);
		switch(pgmhdr.type) {
		case PGM_SPM_PCKT:
			type_tree = proto_item_add_subtree(tf, ett_pgm_spm);

			proto_tree_add_uint_format(type_tree, hf_pgm_spm_sqn, tvb, 
				offset, 4, spm.sqn, "Sequence Number: 0x%x", spm.sqn);
			proto_tree_add_uint_format(type_tree, hf_pgm_spm_sqn, tvb, 
				offset+4, 4, spm.trail, "Trailing Edge Sequence Number: 0x%x", 
				spm.trail);
			proto_tree_add_uint_format(type_tree, hf_pgm_spm_sqn, tvb, 
				offset+8, 4, spm.lead, "Leading Edge Sequence Number: 0x%x", 
				spm.lead);
			proto_tree_add_uint_format(type_tree, hf_pgm_spm_sqn, tvb, 
				offset+10, 2, spm.path_afi, 
				"Network Layer Address (Family Indicator): 0x%x", 
				spm.path_afi);
			proto_tree_add_uint_format(type_tree, hf_pgm_spm_sqn, tvb, 
				offset+12, 2, spm.res, "Reserved: 0x%x", spm.res);
			inaddr.s_addr = htonl(spm.path);
			hp = gethostbyaddr((const char *)&inaddr, 
				sizeof(struct in_addr), AF_INET);
			if (hp)
				sprintf(buf, "Network Layer Address Path: %s (%s)",
					inet_ntoa(inaddr), hp->h_name);
			else
				sprintf(buf, "Network Layer Address Path: %s",
					inet_ntoa(inaddr));
			proto_tree_add_uint_format(type_tree, hf_pgm_spm_sqn, tvb, 
				offset+14, 4, spm.sqn, "%s", buf);

			if ((pgmhdr.opts & PGM_OPT) == FALSE)
				break;
			offset += plen;

			dissect_pgmopts(tvb, offset, type_tree);

			break;

		case PGM_RDATA_PCKT:
		case PGM_ODATA_PCKT:
			type_tree = proto_item_add_subtree(tf, ett_pgm_data);

			proto_tree_add_uint_format(type_tree, hf_pgm_spm_sqn, tvb, 
				offset, 4, data.sqn, "Sequence Number: 0x%x", spm.sqn);
			proto_tree_add_uint_format(type_tree, hf_pgm_spm_sqn, tvb, 
				offset+4, 4, data.trail, "Trailing Edge Sequence Number: 0x%x", 
				data.trail);

			if ((pgmhdr.opts & PGM_OPT) == TRUE) {
				offset += plen;;
				dissect_pgmopts(tvb, offset, type_tree);
			}
			/*
			 * Now see if there are any sub-dissectors, of so call them
			 */
			decode_tcp_ports(tvb, offset, pinfo, tree, 
				pgmhdr.sport, pgmhdr.dport);
			break;

		case PGM_NAK_PCKT:
		case PGM_NNAK_PCKT:
		case PGM_NCF_PCKT:
			type_tree = proto_item_add_subtree(tf, ett_pgm_nak);

			proto_tree_add_uint(type_tree, hf_pgm_nak_sqn, tvb, 
				offset, 4, nak.sqn);
			proto_tree_add_uint(type_tree, hf_pgm_nak_srcafi, tvb, 
				offset+4, 2, nak.src_afi);
			proto_tree_add_uint(type_tree, hf_pgm_nak_srcres, tvb, 
				offset+6, 2, nak.src_res);

			inaddr.s_addr = htonl(nak.src);
			hp = gethostbyaddr((const char *)&inaddr, 
				sizeof(struct in_addr), AF_INET);
			if (hp)
				sprintf(buf, "Source Network Layer Address: %s (%s)",
					inet_ntoa(inaddr), hp->h_name);
			else
				sprintf(buf, "Source Network Layer Address Path: %s",
					inet_ntoa(inaddr));
			proto_tree_add_uint_format(type_tree, hf_pgm_nak_src, tvb, 
				offset+8, 4, nak.src, "%s", buf);

			proto_tree_add_uint_format(type_tree, hf_pgm_nak_grpafi, tvb, 
				offset+12, 2, nak.grp_afi, 
				"Multicast group Network Layer Address (Family Indicator):"
				" 0x%x", nak.grp_afi);
			proto_tree_add_uint_format(type_tree, hf_pgm_nak_grpres, tvb, 
				offset+14, 2, nak.grp_res, "Reserved: 0x%x", nak.grp_res);

			inaddr.s_addr = htonl(nak.grp);
			proto_tree_add_uint_format(type_tree, hf_pgm_nak_grp, tvb, 
				offset+16, 4, nak.grp, 
				"Multicast group Network Layer Address Path: %s",
				inet_ntoa(inaddr));

			if ((pgmhdr.opts & PGM_OPT) == FALSE)
				break;
			offset += plen;

			dissect_pgmopts(tvb, offset, type_tree);

			break;
		}

	}
	pktname = NULL;
}
static const true_false_string opts_present = {      
	"Present",
	"Not Present" 
};

/* Register all the bits needed with the filtering engine */
void 
proto_register_pgm(void)
{
  static hf_register_info hf[] = {
    { &hf_pgm_main_sport,
      { "Source Port", "pgm.hdr.sport", FT_UINT16, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_main_dport,
      { "Destination Port", "pgm.hdr.dport", FT_UINT16, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_main_type,
      { "Type", "pgm.hdr.type", FT_UINT8, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_main_opts,
      { "Options", "pgm.hdr.opts", FT_UINT8, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_main_opts_opt,
      { "Options", "pgm.hdr.opts.opt", FT_BOOLEAN, BASE_HEX, 
	  TFS(&opts_present), PGM_OPT, "", HFILL }},
    { &hf_pgm_main_opts_netsig,
      { "Network Significant Options", "pgm.hdr.opts.netsig", 
	  FT_BOOLEAN, BASE_HEX, TFS(&opts_present), PGM_OPT_NETSIG, "", HFILL }},
    { &hf_pgm_main_opts_varlen,
      { "Variable lenght Parity Packet Option", "pgm.hdr.opts.varlen", 
	  FT_BOOLEAN, BASE_HEX, TFS(&opts_present), PGM_OPT_VAR_PKTLEN, "", HFILL }},
    { &hf_pgm_main_opts_parity,
      { "Parity", "pgm.hdr.opts.parity", FT_BOOLEAN, BASE_HEX, 
	  TFS(&opts_present), PGM_OPT_PARITY, "", HFILL }},
    { &hf_pgm_main_cksum,
      { "Checksum", "pgm.hdr.cksum", FT_UINT8, BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_main_gsi,
      { "Global Source Identifier", "pgm.hdr.gsi", FT_BYTES, BASE_HEX, 
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_main_tsdulen,
      { "Transport Service Data Unit", "pgm.hdr.tsdulen", FT_UINT8, BASE_HEX,
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_spm_sqn,
      { "Sequence number", "pgm.spm.sqn", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_spm_trail,
      { "Trailing Edge Sequence Number", "pgm.spm.trail", FT_UINT32, BASE_HEX, 
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_spm_lead,
      { "Leading Edge Sequence Number", "pgm.spm.lead", FT_UINT32, BASE_HEX, 
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_spm_pathafi,
      { "NLA AFI (IPv4 is set to 1)", "pgm.spm.pathafi", FT_UINT16, BASE_HEX, 
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_spm_res,
      { "Reserved", "pgm.spm.res", FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_spm_path,
      { "Path NLA", "pgm.spm.path", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_data_sqn,
      { "Data Packet Sequence Number", "pgm.data.sqn", FT_UINT32, BASE_HEX, 
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_data_trail,
      { "Trailing Edge Sequence Number", "pgm.data.trail", FT_UINT32, BASE_HEX, 
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_nak_sqn,
      { "Requested Sequence Number", "pgm.nak.sqn", FT_UINT32, BASE_HEX, 
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_nak_srcafi,
      { "Source Network Layer Address (Family Indicator)", "pgm.nak.srcafi", 
	  FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_nak_srcres,
      { "Reserved", "pgm.nak.srcres", FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_nak_src,
      { "Source NLA", "pgm.nak.src", FT_UINT32, BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_nak_grpafi,
      { "Multicast group AFI", "pgm.nak.grpafi", FT_UINT16, BASE_HEX, 
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_nak_grpres,
      { "Reserved", "pgm.nak.grpres", FT_UINT16, BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_nak_grp,
      { "Multicast Group NLA", "pgm.nak.grp", FT_UINT32, BASE_HEX, 
	  NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_type,
      { "Type", "pgm.opts.type", FT_UINT8, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_opt_len,
      { "Length", "pgm.opts.len", FT_UINT8, BASE_DEC, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_opt_tlen,
      { "Total Length", "pgm.opts.tlen", FT_UINT16, BASE_DEC, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_genopt_type,
      { "Type", "pgm.genopts.type", FT_UINT8, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_genopt_len,
      { "Length", "pgm.genopts.len", FT_UINT8, BASE_DEC, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_genopt_opx,
      { "Option Extensibility Bits", "pgm.genopts.opx", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_genopt_res,
      { "Reserved", "pgm.genopts.opx", FT_UINT8, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_opt_parity_prm_type,
      { "Type", "pgm.parity_prm.type", FT_UINT8, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_opt_parity_prm_len,
      { "Length", "pgm.parity_prm.len", FT_UINT8, BASE_DEC, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_opt_parity_prm_opx,
      { "Option Extensibility Bits", "pgm.opts.parity_prm.opx", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_parity_prm_po,
      { "Pro-Active Parity", "pgm.opts.parity_prm.op", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_parity_prm_prmtgsz,
      { "Transmission Group Size", "pgm.opts.parity_prm.prm_grp", FT_UINT32, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_join_type,
      { "Type", "pgm.opts.join.type", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_join_len,
      { "Length", "pgm.opts.join.res", FT_UINT8, BASE_DEC, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_join_opx,
      { "Option Extensibility Bits", "pgm.opts.join.opx", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_join_res,
      { "Reserved", "pgm.opts.join.res", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_join_minjoin,
      { "Minimum Sequence Number", "pgm.opts.join.min_join", FT_UINT32, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_parity_grp_type,
      { "Type", "pgm.parity_grp.type", FT_UINT8, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_opt_parity_grp_len,
      { "Length", "pgm.parity_grp.len", FT_UINT8, BASE_DEC, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_opt_parity_grp_opx,
      { "Option Extensibility Bits", "pgm.opts.parity_prm.opx", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_parity_grp_res,
      { "Reserved", "pgm.opts.parity_prm.op", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_parity_grp_prmgrp,
      { "Transmission Group Size", "pgm.opts.parity_prm.prm_grp", FT_UINT32, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_nak_type,
      { "Type", "pgm.opt.nak.type", FT_UINT8, BASE_HEX, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_opt_nak_len,
      { "Length", "pgm.opts.nak.len", FT_UINT8, BASE_DEC, NULL, 0x0, 
		"", HFILL }},
    { &hf_pgm_opt_nak_opx,
      { "Option Extensibility Bits", "pgm.opts.nak.opx", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_nak_res,
      { "Reserved", "pgm.opts.nak.op", FT_UINT8, 
		  BASE_HEX, NULL, 0x0, "", HFILL }},
    { &hf_pgm_opt_nak_list,
      { "List", "pgm.opts.nak.list", FT_BYTES, BASE_HEX, NULL, 0x0, "", HFILL }},
  };
  static gint *ett[] = {
    &ett_pgm,
	&ett_pgm_optbits,
	&ett_pgm_spm,
	&ett_pgm_data,
	&ett_pgm_nak,
	&ett_pgm_opts,
	&ett_pgm_opts_join,
	&ett_pgm_opts_parityprm,
	&ett_pgm_opts_paritygrp,
	&ett_pgm_opts_naklist,
  };

  proto_pgm = proto_register_protocol("Pragmatic General Multicast",
				       "PGM", "pgm");

  proto_register_field_array(proto_pgm, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}

/* The registration hand-off routine */
void
proto_reg_handoff_pgm(void)
{
  dissector_add("ip.proto", IP_PROTO_PGM, dissect_pgm, proto_pgm);

}
