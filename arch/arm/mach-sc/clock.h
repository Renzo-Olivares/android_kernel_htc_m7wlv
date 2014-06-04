/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
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
 */

#undef debug
#define debug(format, arg...) pr_debug("clk: " "@@@%s: " format, __func__, ## arg)
#define debug0(format, arg...)
#define debug2(format, arg...) pr_debug("clk: " "@@@%s: " format, __func__, ## arg)

struct clk_sel {
	int nr_sources;
	u32 sources[];
};

struct clk_reg {
	u32 reg;
	
	u32 mask;
};

struct clk_regs {
	int id;
	const char *name;
	struct clk_reg enb, div, sel;

	
	int nr_sources;
	struct clk *sources[];
};

struct clk_ops {
	int (*set_rate) (struct clk * c, unsigned long rate);
	unsigned long (*get_rate) (struct clk * c);
	unsigned long (*round_rate) (struct clk * c, unsigned long rate);
	int (*set_parent) (struct clk * c, struct clk * parent);
};


struct clk {
	struct module *owner;
	struct clk *parent;
	int usage;
	unsigned long rate;
	struct clk_ops *ops;
	int (*enable) (struct clk *, int enable, unsigned long *);

	const struct clk_regs *regs;
#if defined(CONFIG_DEBUG_FS)
	struct dentry *dent;	
#endif
};

#define MAX_DIV							(1000)

#define SCI_CLK_ADD(ID, RATE, ENB, ENB_BIT, DIV, DIV_MSK, SEL, SEL_MSK, NR_CLKS, ...)								\
static const struct clk_regs REGS_##ID = {  \
	.name = #ID,                            \
	.id = 0,                                \
	.enb = {                                \
		.reg = (u32)ENB,.mask = ENB_BIT,      	\
		},                                  \
	.div = {                                \
		.reg = (u32)DIV,.mask = DIV_MSK,			\
		},                                  \
	.sel = {                                \
		.reg = (u32)SEL,.mask = SEL_MSK,			\
		},                                  \
	.nr_sources = NR_CLKS,                  \
	.sources = {__VA_ARGS__},               \
};                                          \
static struct clk ID = {              		\
	.owner = THIS_MODULE,                   \
	.parent = 0,                            \
	.usage = 0,                             \
	.rate = RATE,                           \
	.regs = &REGS_##ID,                     \
	.ops = 0,                               \
	.enable = 0,                            \
};                                          \
const struct clk_lookup __clkinit1 CLK_LK_##ID = { \
	.dev_id = 0,							\
	.con_id = #ID,                          \
	.clk = &ID,                       		\
};                                       	\

int __init sci_clk_register(struct clk_lookup *cl);

#define __clkinit0	__section(.rodata.clkinit0)
#define __clkinit1	__section(.rodata.clkinit1)
#define __clkinit2	__section(.rodata.clkinit2)
