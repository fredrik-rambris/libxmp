/* Extended Module Player
 * Copyright (C) 1996-2012 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "med.h"
#include "load.h"


static int mmd3_test (FILE *, char *, const int);
static int mmd3_load (struct xmp_context *, FILE *, const int);

struct xmp_loader_info mmd3_loader = {
	"MMD2/3",
	"OctaMED v5/OctaMED SS",
	mmd3_test,
	mmd3_load
};

static int mmd3_test(FILE *f, char *t, const int start)
{
	char id[4];
	uint32 offset, len;

	if (fread(id, 1, 4, f) < 4)
		return -1;

	if (memcmp(id, "MMD2", 4) && memcmp(id, "MMD3", 4))
		return -1;

	fseek(f, 28, SEEK_CUR);
	offset = read32b(f);		/* expdata_offset */
	
	if (offset) {
		fseek(f, start + offset + 44, SEEK_SET);
		offset = read32b(f);
		len = read32b(f);
		fseek(f, start + offset, SEEK_SET);
		read_title(f, t, len);
	} else {
		read_title(f, t, 0);
	}

	return 0;
}

static int bpm_on, bpmlen, med_8ch;

static int mmd3_load(struct xmp_context *ctx, FILE *f, const int start)
{
	struct xmp_player_context *p = &ctx->p;
	struct xmp_mod_context *m = &p->m;
	int i, j, k;
	struct MMD0 header;
	struct MMD2song song;
	struct MMD1Block block;
	struct InstrHdr instr;
	struct SynthInstr synth;
	struct InstrExt exp_smp;
	struct MMD0exp expdata;
	struct xxm_event *event;
	int ver = 0;
	int smp_idx = 0;
	uint8 e[4];
	int song_offset;
	int seqtable_offset;
	int trackvols_offset;
	int trackpans_offset;
	int blockarr_offset;
	int smplarr_offset;
	int expdata_offset;
	int expsmp_offset;
	int songname_offset;
	int iinfo_offset;
	int playseq_offset;
	int pos;

	LOAD_INIT();

	fread(&header.id, 4, 1, f);

	ver = *((char *)&header.id + 3) - '1' + 1;

	_D(_D_WARN "load header");
	header.modlen = read32b(f);
	song_offset = read32b(f);
	_D(_D_INFO "song_offset = 0x%08x", song_offset);
	read16b(f);
	read16b(f);
	blockarr_offset = read32b(f);
	_D(_D_INFO "blockarr_offset = 0x%08x", blockarr_offset);
	read32b(f);
	smplarr_offset = read32b(f);
	_D(_D_INFO "smplarr_offset = 0x%08x", smplarr_offset);
	read32b(f);
	expdata_offset = read32b(f);
	_D(_D_INFO "expdata_offset = 0x%08x", expdata_offset);
	read32b(f);
	header.pstate = read16b(f);
	header.pblock = read16b(f);
	header.pline = read16b(f);
	header.pseqnum = read16b(f);
	header.actplayline = read16b(f);
	header.counter = read8(f);
	header.extra_songs = read8(f);

	/*
	 * song structure
	 */
	_D(_D_WARN "load song");
	fseek(f, start + song_offset, SEEK_SET);
	for (i = 0; i < 63; i++) {
		song.sample[i].rep = read16b(f);
		song.sample[i].replen = read16b(f);
		song.sample[i].midich = read8(f);
		song.sample[i].midipreset = read8(f);
		song.sample[i].svol = read8(f);
		song.sample[i].strans = read8s(f);
	}
	song.numblocks = read16b(f);
	song.songlen = read16b(f);
	_D(_D_INFO "song.songlen = %d", song.songlen);
	seqtable_offset = read32b(f);
	read32b(f);
	trackvols_offset = read32b(f);
	song.numtracks = read16b(f);
	song.numpseqs = read16b(f);
	trackpans_offset = read32b(f);
	song.flags3 = read32b(f);
	song.voladj = read16b(f);
	song.channels = read16b(f);
	song.mix_echotype = read8(f);
	song.mix_echodepth = read8(f);
	song.mix_echolen = read16b(f);
	song.mix_stereosep = read8(f);

	fseek(f, 223, SEEK_CUR);

	song.deftempo = read16b(f);
	song.playtransp = read8(f);
	song.flags = read8(f);
	song.flags2 = read8(f);
	song.tempo2 = read8(f);
	for (i = 0; i < 16; i++)
		read8(f);		/* reserved */
	song.mastervol = read8(f);
	song.numsamples = read8(f);

	/*
	 * read sequence
	 */
	fseek(f, start + seqtable_offset, SEEK_SET);
	playseq_offset = read32b(f);
	fseek(f, start + playseq_offset, SEEK_SET);
	fseek(f, 32, SEEK_CUR);	/* skip name */
	read32b(f);
	read32b(f);
	m->xxh->len = read16b(f);
	for (i = 0; i < m->xxh->len; i++)
		m->xxo[i] = read16b(f);

	/*
	 * convert header
	 */
	m->c4rate = C4_NTSC_RATE;
	m->quirk |= song.flags & FLAG_STSLIDE ? 0 : XMP_QRK_VSALL;
	med_8ch = song.flags & FLAG_8CHANNEL;
	bpm_on = song.flags2 & FLAG2_BPM;
	bpmlen = 1 + (song.flags2 & FLAG2_BMASK);
	m->quirk |= XMP_QRK_MEDBPM;

	/* From the OctaMEDv4 documentation:
	 *
	 * In 8-channel mode, you can control the playing speed more
	 * accurately (to techies: by changing the size of the mix buffer).
	 * This can be done with the left tempo gadget (values 1-10; the
	 * lower, the faster). Values 11-240 are equivalent to 10.
	 */

	m->xxh->tpo = song.tempo2;
	m->xxh->bpm = med_8ch ?
			mmd_get_8ch_tempo(song.deftempo) :
			(bpm_on ? song.deftempo / bpmlen : song.deftempo);

	m->xxh->pat = song.numblocks;
	m->xxh->ins = song.numsamples;
	//m->xxh->smp = m->xxh->ins;
	m->xxh->rst = 0;
	m->xxh->chn = 0;
	m->name[0] = 0;

	/*
	 * Obtain number of samples from each instrument
	 */
	m->xxh->smp = 0;
	for (i = 0; i < m->xxh->ins; i++) {
		uint32 smpl_offset;
		int16 type;
		fseek(f, start + smplarr_offset + i * 4, SEEK_SET);
		smpl_offset = read32b(f);
		if (smpl_offset == 0)
			continue;
		fseek(f, start + smpl_offset, SEEK_SET);
		read32b(f);				/* length */
		type = read16b(f);
		if (type == -1) {			/* type is synth? */
			fseek(f, 14, SEEK_CUR);
			m->xxh->smp += read16b(f);		/* wforms */
		} else {
			m->xxh->smp++;
		}
	}

	/*
	 * expdata
	 */
	_D(_D_WARN "load expdata");
	expdata.s_ext_entries = 0;
	expdata.s_ext_entrsz = 0;
	expdata.i_ext_entries = 0;
	expdata.i_ext_entrsz = 0;
	expsmp_offset = 0;
	iinfo_offset = 0;
	if (expdata_offset) {
		fseek(f, start + expdata_offset, SEEK_SET);
		read32b(f);
		expsmp_offset = read32b(f);
		_D(_D_INFO "expsmp_offset = 0x%08x", expsmp_offset);
		expdata.s_ext_entries = read16b(f);
		expdata.s_ext_entrsz = read16b(f);
		read32b(f);
		read32b(f);
		iinfo_offset = read32b(f);
		_D(_D_INFO "iinfo_offset = 0x%08x", iinfo_offset);
		expdata.i_ext_entries = read16b(f);
		expdata.i_ext_entrsz = read16b(f);
		read32b(f);
		read32b(f);
		read32b(f);
		read32b(f);
		songname_offset = read32b(f);
		_D(_D_INFO "songname_offset = 0x%08x", songname_offset);
		expdata.songnamelen = read32b(f);
		fseek(f, start + songname_offset, SEEK_SET);
		_D(_D_INFO "expdata.songnamelen = %d", expdata.songnamelen);
		for (i = 0; i < expdata.songnamelen; i++) {
			if (i >= XMP_NAMESIZE)
				break;
			m->name[i] = read8(f);
		}
	}

	/*
	 * Quickly scan patterns to check the number of channels
	 */
	_D(_D_WARN "find number of channels");

	for (i = 0; i < m->xxh->pat; i++) {
		int block_offset;

		fseek(f, start + blockarr_offset + i * 4, SEEK_SET);
		block_offset = read32b(f);
		_D(_D_INFO "block %d block_offset = 0x%08x", i, block_offset);
		if (block_offset == 0)
			continue;
		fseek(f, start + block_offset, SEEK_SET);

		block.numtracks = read16b(f);
		block.lines = read16b(f);

		if (block.numtracks > m->xxh->chn)
			m->xxh->chn = block.numtracks;
	}

	m->xxh->trk = m->xxh->pat * m->xxh->chn;

	if (ver == 2)
	    set_type(m, "MMD%c (OctaMED v5)", '0' + ver);
	else
	    set_type(m, "MMD%c (OctaMED Soundstudio)", '0' + ver);

	MODULE_INFO();

	_D(_D_INFO "BPM mode: %s (length = %d)", bpm_on ? "on" : "off", bpmlen);
	_D(_D_INFO "Song transpose : %d", song.playtransp);
	_D(_D_INFO "Stored patterns: %d", m->xxh->pat);

	/*
	 * Read and convert patterns
	 */
	_D(_D_WARN "read patterns");
	PATTERN_INIT();

	for (i = 0; i < m->xxh->pat; i++) {
		int block_offset;

		fseek(f, start + blockarr_offset + i * 4, SEEK_SET);
		block_offset = read32b(f);
		if (block_offset == 0)
			continue;
		fseek(f, start + block_offset, SEEK_SET);

		block.numtracks = read16b(f);
		block.lines = read16b(f);
		read32b(f);

		PATTERN_ALLOC(i);

		m->xxp[i]->rows = block.lines + 1;
		TRACK_ALLOC(i);

		for (j = 0; j < m->xxp[i]->rows; j++) {
			for (k = 0; k < block.numtracks; k++) {
				e[0] = read8(f);
				e[1] = read8(f);
				e[2] = read8(f);
				e[3] = read8(f);

				event = &EVENT(i, k, j);
				event->note = e[0] & 0x7f;
				if (event->note)
					event->note += 12 + song.playtransp;
				event->ins = e[1] & 0x3f;
				event->fxt = e[2];
				event->fxp = e[3];
				mmd_xlat_fx(event, bpm_on, bpmlen, med_8ch);
			}
		}
	}

	m->med_vol_table = calloc(sizeof(uint8 *), m->xxh->ins);
	m->med_wav_table = calloc(sizeof(uint8 *), m->xxh->ins);

	/*
	 * Read and convert instruments and samples
	 */
	_D(_D_WARN "read instruments");
	INSTRUMENT_INIT();

	_D(_D_INFO "Instruments: %d", m->xxh->ins);

	for (smp_idx = i = 0; i < m->xxh->ins; i++) {
		int smpl_offset;
		int t;
		char name[40] = "";

		fseek(f, start + smplarr_offset + i * 4, SEEK_SET);
		smpl_offset = read32b(f);

		_D(_D_INFO "sample %d smpl_offset = 0x%08x", i, smpl_offset);

		if (smpl_offset == 0)
			continue;

		fseek(f, start + smpl_offset, SEEK_SET);
		instr.length = read32b(f);
		instr.type = read16b(f);

		pos = ftell(f);

		if (expdata_offset && i < expdata.i_ext_entries) {
			fseek(f, iinfo_offset + i * expdata.i_ext_entrsz, SEEK_SET);
			fread(name, 40, 1, f);

			t = (instr.type + 2) & ~(S_16 | STEREO);
			_D(_D_INFO "[%2x] %-40.40s %s", i, name,
				t <= MMD_INST_TYPES ? mmd_inst_type[t] : "???");
		}

		exp_smp.finetune = 0;
		if (expdata_offset && i < expdata.s_ext_entries) {
			fseek(f, expsmp_offset + i * expdata.s_ext_entrsz,
							SEEK_SET);
			exp_smp.hold = read8(f);
			exp_smp.decay = read8(f);
			exp_smp.suppress_midi_off = read8(f);
			exp_smp.finetune = read8(f);

			if (expdata.s_ext_entrsz > 4) {	/* Octamed V5 */
				exp_smp.default_pitch = read8(f);
				exp_smp.instr_flags = read8(f);
			}
		}

		fseek(f, pos, SEEK_SET);

		if (instr.type == -2) {			/* Hybrid */
			int length, type;
			int pos = ftell(f);

			synth.defaultdecay = read8(f);
			fseek(f, 3, SEEK_CUR);
			synth.rep = read16b(f);
			synth.replen = read16b(f);
			synth.voltbllen = read16b(f);
			synth.wftbllen = read16b(f);
			synth.volspeed = read8(f);
			synth.wfspeed = read8(f);
			synth.wforms = read16b(f);
			fread(synth.voltbl, 1, 128, f);;
			fread(synth.wftbl, 1, 128, f);;

			fseek(f, pos - 6 + read32b(f), SEEK_SET);
			length = read32b(f);
			type = read16b(f);

			m->xxi[i] = calloc(sizeof(struct xxm_instrument), 1);
			m->xxih[i].nsm = 1;
			m->xxih[i].vts = synth.volspeed;
			m->xxih[i].wts = synth.wfspeed;
			m->xxi[i][0].pan = 0x80;
			m->xxi[i][0].vol = song.sample[i].svol;
			m->xxi[i][0].xpo = song.sample[i].strans;
			m->xxi[i][0].sid = smp_idx;
			m->xxi[i][0].fin = exp_smp.finetune;
			m->xxs[smp_idx].len = length;
			m->xxs[smp_idx].lps = 2 * song.sample[i].rep;
			m->xxs[smp_idx].lpe = m->xxs[smp_idx].lps +
						2 * song.sample[i].replen;
			m->xxs[smp_idx].flg = song.sample[i].replen > 1 ?
						XMP_SAMPLE_LOOP : 0;

			_D(_D_INFO "  %05x %05x %05x %02x %+3d %+1d",
				       m->xxs[smp_idx].len, m->xxs[smp_idx].lps,
				       m->xxs[smp_idx].lpe, m->xxi[i][0].vol,
				       m->xxi[i][0].xpo,
				       m->xxi[i][0].fin >> 4);

			xmp_drv_loadpatch(ctx, f, smp_idx, m->c4rate, 0,
					&m->xxs[smp_idx], NULL);

			smp_idx++;

			m->med_vol_table[i] = calloc(1, synth.voltbllen);
			memcpy(m->med_vol_table[i], synth.voltbl, synth.voltbllen);

			m->med_wav_table[i] = calloc(1, synth.wftbllen);
			memcpy(m->med_wav_table[i], synth.wftbl, synth.wftbllen);

			continue;
		}

		if (instr.type == -1) {			/* Synthetic */
			int pos = ftell(f);

			synth.defaultdecay = read8(f);
			fseek(f, 3, SEEK_CUR);
			synth.rep = read16b(f);
			synth.replen = read16b(f);
			synth.voltbllen = read16b(f);
			synth.wftbllen = read16b(f);
			synth.volspeed = read8(f);
			synth.wfspeed = read8(f);
			synth.wforms = read16b(f);
			fread(synth.voltbl, 1, 128, f);;
			fread(synth.wftbl, 1, 128, f);;
			for (j = 0; j < 64; j++)
				synth.wf[j] = read32b(f);

			_D(_D_INFO "  VS:%02x WS:%02x WF:%02x %02x %+3d %+1d",
					synth.volspeed, synth.wfspeed,
					synth.wforms & 0xff,
					song.sample[i].svol,
					song.sample[i].strans,
					exp_smp.finetune);

			if (synth.wforms == 0xffff)
				continue;

			m->xxi[i] = calloc(sizeof(struct xxm_instrument),
							synth.wforms);
			m->xxih[i].nsm = synth.wforms;
			m->xxih[i].vts = synth.volspeed;
			m->xxih[i].wts = synth.wfspeed;

			for (j = 0; j < synth.wforms; j++) {
				m->xxi[i][j].pan = 0x80;
				m->xxi[i][j].vol = song.sample[i].svol;
				m->xxi[i][j].xpo = song.sample[i].strans - 24;
				m->xxi[i][j].sid = smp_idx;
				m->xxi[i][j].fin = exp_smp.finetune;

				fseek(f, pos - 6 + synth.wf[j], SEEK_SET);

				m->xxs[smp_idx].len = read16b(f) * 2;
				m->xxs[smp_idx].lps = 0;
				m->xxs[smp_idx].lpe = m->xxs[smp_idx].len;
				m->xxs[smp_idx].flg = XMP_SAMPLE_LOOP;

				xmp_drv_loadpatch(ctx, f, smp_idx, m->c4rate,
					0, &m->xxs[smp_idx], NULL);


				smp_idx++;
			}

			m->med_vol_table[i] = calloc(1, synth.voltbllen);
			memcpy(m->med_vol_table[i], synth.voltbl, synth.voltbllen);

			m->med_wav_table[i] = calloc(1, synth.wftbllen);
			memcpy(m->med_wav_table[i], synth.wftbl, synth.wftbllen);

			continue;
		}

		if ((instr.type & ~(S_16 | STEREO)) != 0)
			continue;

		/* instr type is sample */
		m->xxi[i] = calloc(sizeof(struct xxm_instrument), 1);
		m->xxih[i].nsm = 1;

		m->xxi[i][0].vol = song.sample[i].svol;
		m->xxi[i][0].pan = 0x80;
		m->xxi[i][0].xpo = song.sample[i].strans;
		m->xxi[i][0].sid = smp_idx;
		m->xxi[i][0].fin = exp_smp.finetune << 4;

		m->xxs[smp_idx].len = instr.length;
		m->xxs[smp_idx].lps = 2 * song.sample[i].rep;
		m->xxs[smp_idx].lpe = m->xxs[smp_idx].lps + 2 *
						song.sample[i].replen;
		m->xxs[smp_idx].flg = 0;
		if (song.sample[i].replen > 1) {
			m->xxs[smp_idx].flg |= XMP_SAMPLE_LOOP;
		}
		if (instr.type & S_16) {
			m->xxs[smp_idx].flg |= XMP_SAMPLE_16BIT;
			m->xxs[smp_idx].len >>= 1;
			m->xxs[smp_idx].lps >>= 1;
			m->xxs[smp_idx].lpe >>= 1;
		}

		/* STEREO means that this is a stereo sample. The sample
		 * is not interleaved. The left channel comes first,
		 * followed by the right channel. Important: Length
		 * specifies the size of one channel only! The actual memory
		 * usage for both samples is length * 2 bytes.
		 */

		_D(_D_INFO "  %05x%c%05x %05x %02x %+3d %+1d ",
				m->xxs[smp_idx].len,
				m->xxs[smp_idx].flg & XMP_SAMPLE_16BIT ? '+' : ' ',
				m->xxs[smp_idx].lps,
				m->xxs[smp_idx].lpe,
				m->xxi[i][0].vol,
				m->xxi[i][0].xpo,
				m->xxi[i][0].fin >> 4);

		fseek(f, start + smpl_offset + 6, SEEK_SET);
		xmp_drv_loadpatch(ctx, f, smp_idx, m->c4rate, XMP_SMP_BIGEND,
				  &m->xxs[smp_idx], NULL);

		smp_idx++;
	}

	fseek(f, start + trackvols_offset, SEEK_SET);
	for (i = 0; i < m->xxh->chn; i++)
		m->xxc[i].vol = read8(f);;

	if (trackpans_offset) {
		fseek(f, start + trackpans_offset, SEEK_SET);
		for (i = 0; i < m->xxh->chn; i++) {
			int p = 8 * read8s(f);
			m->xxc[i].pan = 0x80 + (p > 127 ? 127 : p);
		}
	} else {
		for (i = 0; i < m->xxh->chn; i++)
			m->xxc[i].pan = 0x80;
	}

	return 0;
}
