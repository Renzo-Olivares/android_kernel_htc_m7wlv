/* ==========================================================================
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 * 
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 * 
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */

#ifndef __DWC_OTG_DBG_H__
#define __DWC_OTG_DBG_H__


extern uint32_t g_dbg_lvl;
static inline uint32_t SET_DEBUG_LEVEL(const uint32_t new)
{
	uint32_t old = g_dbg_lvl;
	g_dbg_lvl = new;
	return old;
}
#define DBG_CIL		(0x2)
#define DBG_CILV	(0x20)
#define DBG_PCD		(0x4)
#define DBG_PCDV	(0x40)
#define DBG_HCD		(0x8)
#define DBG_HCDV	(0x80)
#define DBG_HCD_URB	(0x800)

#define DBG_ANY		(0xFF)

#define DBG_OFF		0

#define USB_DWC "DWC_otg: "

/** 
 * Print a debug message when the Global debug level variable contains
 * the bit defined in <code>lvl</code>.
 *
 * @param[in] lvl - Debug level, use one of the DBG_ constants above.
 * @param[in] x - like printf
 *
 *    Example:<p>
 * <code>
 *      DWC_DEBUGPL( DBG_ANY, "%s(%p)\n", __func__, _reg_base_addr);
 * </code>
 * <br>
 * results in:<br> 
 * <code>
 * usb-DWC_otg: dwc_otg_cil_init(ca867000)
 * </code>
 */
#ifdef DEBUG

# define DWC_DEBUGPL(lvl, x...) do{ if ((lvl)&g_dbg_lvl)__DWC_DEBUG(USB_DWC x ); }while(0)
# define DWC_DEBUGP(x...)	DWC_DEBUGPL(DBG_ANY, x )

# define CHK_DEBUG_LEVEL(level) ((level) & g_dbg_lvl)

#else

# define DWC_DEBUGPL(lvl, x...) do{}while(0)
# define DWC_DEBUGP(x...)

# define CHK_DEBUG_LEVEL(level) (0)

#endif 
#endif
