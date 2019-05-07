#include <AMReX_MLEBTensorOp.H>
#include <AMReX_EBMultiFabUtil.H>
#include <AMReX_MultiFabUtil.H>
#include <AMReX_MLEBTensor_K.H>

namespace amrex {

MLEBTensorOp::MLEBTensorOp (const Vector<Geometry>& a_geom,
                            const Vector<BoxArray>& a_grids,
                            const Vector<DistributionMapping>& a_dmap,
                            const LPInfo& a_info,
                            const Vector<EBFArrayBoxFactory const*>& a_factory)
{
    define(a_geom, a_grids, a_dmap, a_info, a_factory);
}

MLEBTensorOp::~MLEBTensorOp ()
{}

void
MLEBTensorOp::define (const Vector<Geometry>& a_geom,
                      const Vector<BoxArray>& a_grids,
                      const Vector<DistributionMapping>& a_dmap,
                      const LPInfo& a_info,
                      const Vector<EBFArrayBoxFactory const*>& a_factory)
{
    BL_PROFILE("MLEBTensorOp::define()");

#if 0
    MLABecLaplacian::define(a_geom, a_grids, a_dmap, a_info, a_factory);

    MLABecLaplacian::setScalars(1.0,1.0);

    m_kappa.clear();
    m_kappa.resize(NAMRLevels());
    for (int amrlev = 0; amrlev < NAMRLevels(); ++amrlev) {
        for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
            m_kappa[amrlev][idim].define(amrex::convert(m_grids[amrlev][0],
                                                        IntVect::TheDimensionVector(idim)),
                                         m_dmap[amrlev][0], 1, 0,
                                         MFInfo(), *m_factory[amrlev][0]);
            m_kappa[amrlev][idim].setVal(0.0);
        }
    }
#endif
}

void
MLEBTensorOp::setShearViscosity (int amrlev, const Array<MultiFab const*,AMREX_SPACEDIM>& eta)
{
//    MLABecLaplacian::setBCoeffs(amrlev, eta);
}

void
MLEBTensorOp::setBulkViscosity (int amrlev, const Array<MultiFab const*,AMREX_SPACEDIM>& kappa)
{
#if 0
    for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
        MultiFab::Copy(m_kappa[amrlev][idim], *kappa[idim], 0, 0, 1, 0);
    }
#endif
}

void
MLEBTensorOp::prepareForSolve ()
{
//    MLABecLaplacian::prepareForSolve();
}

void
MLEBTensorOp::apply (int amrlev, int mglev, MultiFab& out, MultiFab& in, BCMode bc_mode,
                   StateMode s_mode, const MLMGBndry* bndry) const
{
#if 0
    BL_PROFILE("MLEBTensorOp::apply()");

    MLABecLaplacian::apply(amrlev, mglev, out, in, bc_mode, s_mode, bndry);

    // Note that we cannot have inhomog bc_mode at mglev>0.
    if (mglev == 0 && bc_mode == BCMode::Inhomogeneous && s_mode == StateMode::Solution)
    {
        applyBCTensor(amrlev, in, bndry);

        const auto dxinv = m_geom[amrlev][0].InvCellSizeArray();

        Array<MultiFab,AMREX_SPACEDIM> const& etamf = m_b_coeffs[amrlev][0];
        Array<MultiFab,AMREX_SPACEDIM> const& kapmf = m_kappa[amrlev];

#ifdef _OPENMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
        {
            FArrayBox fluxfab_tmp[AMREX_SPACEDIM];
            for (MFIter mfi(out, TilingIfNotGPU()); mfi.isValid(); ++mfi)
            {
                const Box& bx = mfi.tilebox();
                Array4<Real> const axfab = out.array(mfi);
                Array4<Real const> const vfab = in.array(mfi);
                AMREX_D_TERM(Array4<Real const> const etaxfab = etamf[0].array(mfi);,
                             Array4<Real const> const etayfab = etamf[1].array(mfi);,
                             Array4<Real const> const etazfab = etamf[2].array(mfi););
                AMREX_D_TERM(Array4<Real const> const kapxfab = kapmf[0].array(mfi);,
                             Array4<Real const> const kapyfab = kapmf[1].array(mfi);,
                             Array4<Real const> const kapzfab = kapmf[2].array(mfi););
                AMREX_D_TERM(Box const xbx = amrex::surroundingNodes(bx,0);,
                             Box const ybx = amrex::surroundingNodes(bx,1);,
                             Box const zbx = amrex::surroundingNodes(bx,2););
                AMREX_D_TERM(fluxfab_tmp[0].resize(xbx,AMREX_SPACEDIM);,
                             fluxfab_tmp[1].resize(ybx,AMREX_SPACEDIM);,
                             fluxfab_tmp[2].resize(zbx,AMREX_SPACEDIM););
                AMREX_D_TERM(Elixir fxeli = fluxfab_tmp[0].elixir();,
                             Elixir fyeli = fluxfab_tmp[1].elixir();,
                             Elixir fzeli = fluxfab_tmp[2].elixir(););
                AMREX_D_TERM(Array4<Real> const fxfab = fluxfab_tmp[0].array();,
                             Array4<Real> const fyfab = fluxfab_tmp[1].array();,
                             Array4<Real> const fzfab = fluxfab_tmp[2].array(););
                AMREX_LAUNCH_HOST_DEVICE_LAMBDA
                ( xbx, txbx,
                  {
                      mltensor_cross_terms_fx(txbx,fxfab,vfab,etaxfab,kapxfab,dxinv);
                  }
                , ybx, tybx,
                  {
                      mltensor_cross_terms_fy(tybx,fyfab,vfab,etayfab,kapyfab,dxinv);
                  }
#if (AMREX_SPACEDIM == 3)
                , zbx, tzbx,
                  {
                      mltensor_cross_terms_fz(tzbx,fzfab,vfab,etazfab,kapzfab,dxinv);
                  }
#endif
                );

                AMREX_LAUNCH_HOST_DEVICE_LAMBDA ( bx, tbx,
                {
                    mltensor_cross_terms(tbx, axfab, AMREX_D_DECL(fxfab,fyfab,fzfab), dxinv);
                });
            }
        }
    }
#endif
}

void
MLEBTensorOp::applyBCTensor (int amrlev, MultiFab& vel, const MLMGBndry* bndry) const
{
#if 0
    const int mglev = 0;
    const int imaxorder = maxorder;
    const auto& bcondloc = *m_bcondloc[amrlev][mglev];
    const auto& maskvals = m_maskvals[amrlev][mglev];

    FArrayBox foofab(Box::TheUnitBox(),3);
    const auto& foo = foofab.array();

    const auto dxinv = m_geom[amrlev][mglev].InvCellSizeArray();
    const Box& domain = m_geom[amrlev][mglev].Domain();

    MFItInfo mfi_info;
    if (Gpu::notInLaunchRegion()) mfi_info.SetDynamic(true);
#ifdef _OPENMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
    for (MFIter mfi(vel, mfi_info); mfi.isValid(); ++mfi)
    {
        const Box& vbx = mfi.validbox();

        const auto& velfab = vel.array(mfi);

        const auto & bdlv = bcondloc.bndryLocs(mfi);
        const auto & bdcv = bcondloc.bndryConds(mfi);

        GpuArray<BoundCond,2*AMREX_SPACEDIM*AMREX_SPACEDIM> bct;
        GpuArray<Real,2*AMREX_SPACEDIM*AMREX_SPACEDIM> bcl;
        for (OrientationIter face; face; ++face) {
            Orientation ori = face();
            const int iface = ori;
            const int idir = ori.coordDir();
            for (int icomp = 0; icomp < AMREX_SPACEDIM; ++icomp) {
                bct[iface*AMREX_SPACEDIM+icomp] = bdcv[icomp][ori];
                bcl[iface*AMREX_SPACEDIM+icomp] = bdlv[icomp][ori];
            }
        }

#if (AMREX_SPACEDIM == 2)
        const auto& mxlo = maskvals[Orientation(0,Orientation::low )].array(mfi);
        const auto& mylo = maskvals[Orientation(1,Orientation::low )].array(mfi);
        const auto& mxhi = maskvals[Orientation(0,Orientation::high)].array(mfi);
        const auto& myhi = maskvals[Orientation(1,Orientation::high)].array(mfi);

        const auto& bvxlo = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(0,Orientation::low )).array(mfi) : foo;
        const auto& bvylo = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(1,Orientation::low )).array(mfi) : foo;
        const auto& bvxhi = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(0,Orientation::high)).array(mfi) : foo;
        const auto& bvyhi = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(1,Orientation::high)).array(mfi) : foo;

        AMREX_FOR_1D ( 4, icorner,
        {
            mltensor_fill_corners(icorner, vbx, velfab,
                                  mxlo, mylo, mxhi, myhi,
                                  bvxlo, bvylo, bvxhi, bvyhi,
                                  bct, bcl, imaxorder, dxinv, domain);
        });
#else
        const auto& mxlo = maskvals[Orientation(0,Orientation::low )].array(mfi);
        const auto& mylo = maskvals[Orientation(1,Orientation::low )].array(mfi);
        const auto& mzlo = maskvals[Orientation(2,Orientation::low )].array(mfi);
        const auto& mxhi = maskvals[Orientation(0,Orientation::high)].array(mfi);
        const auto& myhi = maskvals[Orientation(1,Orientation::high)].array(mfi);
        const auto& mzhi = maskvals[Orientation(2,Orientation::high)].array(mfi);

        const auto& bvxlo = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(0,Orientation::low )).array(mfi) : foo;
        const auto& bvylo = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(1,Orientation::low )).array(mfi) : foo;
        const auto& bvzlo = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(2,Orientation::low )).array(mfi) : foo;
        const auto& bvxhi = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(0,Orientation::high)).array(mfi) : foo;
        const auto& bvyhi = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(1,Orientation::high)).array(mfi) : foo;
        const auto& bvzhi = (bndry != nullptr) ?
            bndry->bndryValues(Orientation(2,Orientation::high)).array(mfi) : foo;

        AMREX_FOR_1D ( 12, iedge,
        {
            mltensor_fill_edges(iedge, vbx, velfab,
                                mxlo, mylo, mzlo, mxhi, myhi, mzhi,
                                bvxlo, bvylo, bvzlo, bvxhi, bvyhi, bvzhi,
                                bct, bcl, imaxorder, dxinv, domain);
        });

        AMREX_FOR_1D ( 8, icorner,
        {
            mltensor_fill_corners(icorner, vbx, velfab,
                                  mxlo, mylo, mzlo, mxhi, myhi, mzhi,
                                  bvxlo, bvylo, bvzlo, bvxhi, bvyhi, bvzhi,
                                  bct, bcl, imaxorder, dxinv, domain);
        });
#endif
    }

#endif
}

}
