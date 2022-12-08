Name: numbirch
Version: 0.0.0
Release: 1
Summary: Numerical kernels and copy-on-write arrays
Vendor: Lawrence Murray <lawrence@indii.org>
License: Apache-2.0
Group: Development/Libraries/C and C++
URL: https://birch.sh
Source0: %{name}-%{version}.tar.gz

%if 0%{?suse_version}
BuildRequires: gcc-c++ autoconf automake libtool eigen3-devel
%endif
%if 0%{?fedora} || 0%{?rhel_version} || 0%{?centos_version}
BuildRequires: gcc-c++ autoconf automake libtool eigen3-devel
%endif

%description
C++ library providing numerical kernels and copy-on-write arrays.

%package -n lib%{name}-0_0_0
Summary: Shared libraries for NumBirch
%description -n lib%{name}-0_0_0
Shared libraries for Numbirch.

%package devel
Summary: Development files for NumBirch
Requires: lib%{name}-0_0_0 == %{version}
%description devel
Development files for Numbirch.

%package devel-static
Summary: Static libraries for NumBirch
Requires: %{name}-devel == %{version}
%description devel-static
Static libraries for Numbirch.

%package -n lib%{name}-cuda-0_0_0
Summary: Shared libraries for NumBirch with CUDA backend
%description -n lib%{name}-cuda-0_0_0
Shared libraries for Numbirch with CUDA backend.

%package cuda-devel
Summary: Development files for NumBirch with CUDA backend
Requires: lib%{name}-cuda-0_0_0 == %{version}
%description cuda-devel
Development files for Numbirch with CUDA backend.

%package cuda-devel-static
Summary: Static libraries for NumBirch with CUDA backend
Requires: %{name}-cuda-devel == %{version}
%description cuda-devel-static
Static libraries for Numbirch with CUDA backend.

%if 0%{?suse_version}
%debug_package
%else
%global debug_package %{nil}
%endif

%prep
%setup -n %{name}-%{version}

%build

# tweaks to link-time optimization flags to avoid RPM lint errors on static
# library builds
%define _lto_cflags -flto -ffat-lto-objects

# environment variables to find CUDA and custom jemalloc build
export PATH=/usr/local/cuda/bin:/usr/local/bin:$PATH
export CPLUS_INCLUDE_PATH=/usr/local/cuda/include:/usr/local/include:$CPLUS_INCLUDE_PATH
export LIBRARY_PATH=/usr/local/cuda/lib64:/usr/local/cuda/lib:/usr/local/lib64:/usr/local/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:/usr/local/cuda/lib:/usr/local/lib64:/usr/local/lib:$LD_LIBRARY_PATH

mkdir -p eigen
cd eigen
../configure --disable-assert --enable-shared --enable-static --enable-eigen --disable-cuda
%make_build
cd ..

mkdir -p cuda
cd cuda
../configure --disable-assert --enable-shared --enable-static --disable-eigen --enable-cuda
%make_build
cd ..

%install

cd eigen
%make_install
cd ..

cd cuda
%make_install
cd ..

%post -n lib%{name}-0_0_0 -p /sbin/ldconfig
%postun -n lib%{name}-0_0_0 -p /sbin/ldconfig

%files -n lib%{name}-0_0_0
%license LICENSE
%{_libdir}/lib%{name}-single-%{version}.so
%{_libdir}/lib%{name}-%{version}.so

%files devel
%license LICENSE
%{_includedir}/%{name}*
%{_libdir}/lib%{name}-single.so
%{_libdir}/lib%{name}.so

%files devel-static
%license LICENSE
%{_libdir}/lib%{name}-single.a
%{_libdir}/lib%{name}.a

%files -n lib%{name}-cuda-0_0_0
%license LICENSE
%{_libdir}/lib%{name}-cuda-single-%{version}.so
%{_libdir}/lib%{name}-cuda-%{version}.so

%files cuda-devel
%license LICENSE
%{_libdir}/lib%{name}-cuda-single.so
%{_libdir}/lib%{name}-cuda.so

%files cuda-devel-static
%license LICENSE
%{_libdir}/lib%{name}-cuda-single.a
%{_libdir}/lib%{name}-cuda.a

%exclude %{_libdir}/lib%{name}-single.la
%exclude %{_libdir}/lib%{name}.la
%exclude %{_libdir}/lib%{name}-cuda-single.la
%exclude %{_libdir}/lib%{name}-cuda.la

%changelog
* Fri Dec 2 2022 Lawrence Murray <lawrence@indii.org> - 1:0.0.0-1
Initial setup.

