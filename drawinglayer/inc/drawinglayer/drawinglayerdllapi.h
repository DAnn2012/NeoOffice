/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_DRAWINGLAYER_DRAWINGLAYERDLLAPI_H
#define INCLUDED_DRAWINGLAYER_DRAWINGLAYERDLLAPI_H

#include <sal/types.h>

#if defined(DRAWINGLAYER_DLLIMPLEMENTATION)
#define DRAWINGLAYER_DLLPUBLIC SAL_DLLPUBLIC_EXPORT
#else
#define DRAWINGLAYER_DLLPUBLIC SAL_DLLPUBLIC_IMPORT
#endif
#define DRAWINGLAYER_DLLPRIVATE SAL_DLLPRIVATE

#if SUPD == 310

namespace rtl
{
	class OString;
	class OStringBuffer;
	class OUString;
	class OUStringBuffer;
	class OUStringHash;
}

using rtl::OString;
using rtl::OStringBuffer;
using rtl::OUString;
using rtl::OUStringBuffer;
using rtl::OUStringHash;

#endif	// SUPD == 310

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
