%define name hpg-variant
%define version 0.4
Name:           %{name}
Version:        %{version}
Release:        1%{?dist}
Summary:        Bioinformatics tool suite for analyzing genomic variations

License:        GPLv2
URL:            http://bioinfo.cipf.es
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  glibc-devel
BuildRequires:  scons
BuildRequires:  libconfig-devel
BuildRequires:  libcurl-devel
BuildRequires:  gsl-devel
BuildRequires:  libxml2-devel
BuildRequires:  zlib-devel

Requires:       libconfig
Requires:       libcurl
Requires:       gsl
Requires:       libxml2
Requires:       zlib

%description
HPG Variant retrieves the effect of genome mutations and allows to conduct analysis

%prep
%setup -q


%build
scons


%install
rm -rf $RPM_BUILD_ROOT

mkdir -p %{buildroot}%{_bindir}/
cp bin/hpg-var-effect %{buildroot}%{_bindir}/
cp bin/hpg-var-gwas   %{buildroot}%{_bindir}/
cp bin/hpg-var-vcf    %{buildroot}%{_bindir}/

mkdir -p %{buildroot}%{_sysconfdir}/%{name}/
cp bin/hpg-variant.conf %{buildroot}%{_sysconfdir}/%{name}/
cp bin/vcf-info-fields.conf %{buildroot}%{_sysconfdir}/%{name}/


%files
%doc
%{_bindir}/*
%{_sysconfdir}/*


%changelog
* Mon Mar 04 2013 Cristina Yenyxe Gonzalez <cgonzalez@cipf.es> - 0.4
- Filter output uses default names, 'your_vcf_file.vcf.filtered' and
  'your_vcf_file.vcf.rejected', 'out' argument reserved for tool output
- Merge tool notifies when a sample appears more than once
- GWAS tools notify when a sample appears more than once
