/*
 * AresBWAPIBot : A bot for SC:BW using BWAPI library;
 * Copyright (C) 2015 Vincent PALANCHER; Florian LEMEASLE
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

/**
 * TETDeclaration is a header that declares a Testable External Type
 * which can is a typedef to the external type, or in a testing
 * build a typedef to a mock of the external type, with the same
 * interface.
 */
#ifdef TESTING
#include TET_TEST_PATH
#else
#include TET_EXTERNAL_PATH
#endif

namespace ares
{
#ifdef TESTING
    typedef BOOST_PP_CAT(testing::Mock, TET_NAME) TET_NAME;
#else
    typedef BWAPI::TET_NAME TET_NAME;
#endif
}
