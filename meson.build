#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

project('DirectFB2-ge', 'c',
        version: '0.0.0')

directfb_dep = dependency('directfb-internal')
drmkms_dep = dependency('libdrm')

moduledir = directfb_dep.get_pkgconfig_variable('moduledir')

ge_dep = dependency('libge')

ge_sources = [
  'ge_2d.c',
  'ge_gfxdriver.c'
]

library('directfb_ge',
        ge_sources,
        dependencies: [directfb_dep, drmkms_dep, ge_dep],
        install: true,
        install_dir: join_paths(moduledir, 'gfxdrivers'))
