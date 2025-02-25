/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * 
 * This file incorporates work covered by the following license notice:
 *
 *   Modified December 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vcl/IconThemeSelector.hxx>

#include <vcl/IconThemeScanner.hxx>
#include <vcl/IconThemeInfo.hxx>

#include <algorithm>

namespace vcl {

/*static*/ const OUString
IconThemeSelector::HIGH_CONTRAST_ICON_THEME_ID("hicontrast");

/*static*/ const OUString
IconThemeSelector::FALLBACK_ICON_THEME_ID("tango");

namespace {

    class SameTheme :
        public std::unary_function<const vcl::IconThemeInfo &, bool>
    {
    private:
        const OUString& m_rThemeId;
    public:
        SameTheme(const OUString &rThemeId) : m_rThemeId(rThemeId) {}
        bool operator()(const vcl::IconThemeInfo &rInfo)
        {
            return m_rThemeId == rInfo.GetThemeId();
        }
    };

bool icon_theme_is_in_installed_themes(const OUString& theme,
        const std::vector<IconThemeInfo>& installedThemes)
{
    return std::find_if(installedThemes.begin(), installedThemes.end(),
               SameTheme(theme)
           ) != installedThemes.end();
}

} // end anonymous namespace

IconThemeSelector::IconThemeSelector()
: mUseHighContrastTheme(false)
{
}

/*static*/ OUString
IconThemeSelector::GetIconThemeForDesktopEnvironment(const OUString& desktopEnvironment)
{
    OUString r;
    if ( desktopEnvironment.equalsIgnoreAsciiCase("tde") ||
         desktopEnvironment.equalsIgnoreAsciiCase("kde") ) {
        r = "crystal";
    }
    else if ( desktopEnvironment.equalsIgnoreAsciiCase("kde4") ) {
        r = "oxygen";
    }
    else if ( desktopEnvironment.equalsIgnoreAsciiCase("MacOSX") ) {
#ifdef USE_JAVA
        r = "tango";
#else	// USE_JAVA
        r = "sifr";
#endif	// USE_JAVA
    }
    else {
        r = FALLBACK_ICON_THEME_ID;
    }
    return r;
}

OUString
IconThemeSelector::SelectIconThemeForDesktopEnvironment(
        const std::vector<IconThemeInfo>& installedThemes,
        const OUString& desktopEnvironment) const
{
    if (!mPreferredIconTheme.isEmpty()) {
        if (icon_theme_is_in_installed_themes(mPreferredIconTheme, installedThemes)) {
            return mPreferredIconTheme;
        }
    }

    OUString themeForDesktop = GetIconThemeForDesktopEnvironment(desktopEnvironment);
    if (icon_theme_is_in_installed_themes(themeForDesktop, installedThemes)) {
        return themeForDesktop;
    }

    return ReturnFallback(installedThemes);
}

OUString
IconThemeSelector::SelectIconTheme(
        const std::vector<IconThemeInfo>& installedThemes,
        const OUString& theme) const
{
    if (mUseHighContrastTheme) {
        if (icon_theme_is_in_installed_themes(HIGH_CONTRAST_ICON_THEME_ID, installedThemes)) {
            return HIGH_CONTRAST_ICON_THEME_ID;
        }
    }

    if (icon_theme_is_in_installed_themes(theme, installedThemes)) {
        return theme;
    }

    return ReturnFallback(installedThemes);
}

void
IconThemeSelector::SetUseHighContrastTheme(bool v)
{
    mUseHighContrastTheme = v;
}

void
IconThemeSelector::SetPreferredIconTheme(const OUString& theme)
{
    mPreferredIconTheme = theme;
}

bool
IconThemeSelector::operator==(const vcl::IconThemeSelector& other) const
{
    if (this == &other) {
        return true;
    }
    if (mPreferredIconTheme != other.mPreferredIconTheme) {
        return false;
    }
    if (mUseHighContrastTheme != other.mUseHighContrastTheme) {
        return false;
    }
    return true;
}

bool
IconThemeSelector::operator!=(const vcl::IconThemeSelector& other) const
{
    return !((*this) == other);
}

/*static*/ OUString
IconThemeSelector::ReturnFallback(const std::vector<IconThemeInfo>& installedThemes)
{
    if (!installedThemes.empty()) {
        return installedThemes.front().GetThemeId();
    }
    else {
        return FALLBACK_ICON_THEME_ID;
    }
}

} /* namespace vcl */

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
