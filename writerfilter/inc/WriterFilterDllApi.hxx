/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



#ifndef INCLUDED_WRITERFILTERDLLAPI_H
#define INCLUDED_WRITERFILTERDLLAPI_H

#include "sal/types.h"

#if defined(WRITERFILTER_DLLIMPLEMENTATION)
#define WRITERFILTER_DLLPUBLIC  SAL_DLLPUBLIC_EXPORT
#else
#define WRITERFILTER_DLLPUBLIC  SAL_DLLPUBLIC_IMPORT
#endif
#define WRITERFILTER_DLLPRIVATE SAL_DLLPRIVATE

#if SUPD == 310

#include <string>

#ifdef MACOSX
// typedef nullptr for pre-C++11 compiler
#include <objc/objc-api.h>
#if !__has_feature(cxx_nullptr)
#define nullptr __DARWIN_NULL
#endif
#endif	// MACOSX

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
using std::string;

#endif	// SUPD == 310

#endif /* INCLUDED_WRITERFILTERDLLAPI_H */
