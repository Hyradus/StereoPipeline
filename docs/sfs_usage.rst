.. _sfs_usage:

Shape-from-shading examples
===========================

ASP provides a tool, named ``sfs`` (:numref:`sfs`), that can improve
the level of detail of DEMs created by ASP or any other source using
*shape-from-shading* (SfS). The tool takes as input one or more images
and cameras, a DEM at roughly the same resolution as the
images, and returns a refined DEM. This chapter shows in a lot of
detail how this tool is to be used.

The modeling approach used by this program can be found in
:cite:`alexandrov2018multiview`.

.. _sfs_overview:

Overview
--------

``sfs`` works with any cameras supported by ASP, for Earth and other
planets. The option ``--sun-positions`` can be used to to specify the
Sun position for each image. For ISIS and CSM cameras, if this option
is not set, the Sun information is read from the camera files.

``sfs`` has been tested thoroughly with Lunar LRO NAC datasets, and
some experiments were done with Mars HiRISE images and with pictures
from Charon, Pluto's moon. As seen later in the text, it returns
reasonable results on the Moon as far as 85 degrees and even 89.6
degrees South.

The ``sfs`` program is sensitive to errors in the position and
orientation of the cameras, the accuracy of the initial DEM, and to
the value of the weights it uses. Yet, with some effort, it can
work quite well.

A tool named ``parallel_sfs`` is provided (:numref:`parallel_sfs`)
that parallelizes ``sfs`` using multiple processes (optionally on
multiple machines) by splitting the input DEM into tiles with padding,
running ``sfs`` on each tile, and then blending the results. It was used
to create DEMs of dimensions 10,000 by 10,000 pixels.

The ``sfs`` program can model position-dependent albedo
(:numref:`sfs_albedo`), exposure values for each camera, shadows in
the input images, and regions in the DEM occluded from the Sun.

.. _sfs_formulation:

Mathematical model
------------------

The tool works by minimizing the cost function


.. math::

   \label{cost}
     \int\!\! \int \! \sum_k \left[ I_k(\phi(x, y)) - T_k A(x, y)
       R_k(\phi(x, y)) \right]^2\,  
     % R_k(\phi(x, y)) \right]^2\,  \\
     + \mu \left\|\nabla^2 \phi(x, y) \right\|^2  
     + \lambda  \left[ \phi(x, y) - \phi_0(x, y) \right]^2
     \, dx\, dy.

Here, :math:`I_k(\phi(x, y))` is the :math:`k`-th camera image
interpolated at pixels obtained by projecting into the camera 3D points
from the terrain :math:`\phi(x, y)`, :math:`T_k` is the :math:`k`-th
image exposure, :math:`A(x, y)` is the per-pixel normalized albedo,
:math:`R_k(\phi(x, y))` is the reflectance computed from the terrain for
:math:`k`-th image, :math:`\left\|\nabla^2 \phi(x, y) \right\|^2` is the
sum of squares of all second-order partial derivatives of :math:`\phi`,
:math:`\mu > 0` is a smoothing term, and :math:`\lambda > 0` determines
how close we should stay to the input terrain :math:`\phi_0` (smaller
:math:`\mu` will show more detail but may introduce some artifacts, and
smaller :math:`\lambda` may allow for more flexibility in optimization
but the terrain may move too far from the input).

We use either the regular Lambertian reflectance model, or the
Lunar-Lambertian model :cite:`mcewen1991photometric`, more
specifically as given in :cite:`lohse2006derivation` (equations (3)
and (4)). Also supported is the Hapke model,
:cite:`johnson2006spectrophotometric`, :cite:`fernando2013surface`,
:cite:`hapke2008bidirectional`, :cite:`hapke1993opposition`. Custom
values for the coefficients of these models can be passed to the
program.

Note that the albedo in the above expression is normalized, its
nominal value is 1.  The image intensity ideally equals the image
exposure times albedo times reflectance, hence the image exposure
absorbs any existing constant multiplier.

It is important to note that the default Lunar-Lambertian model may
not be the right choice for other planetary bodies, hence some
research may be needed to decide the correct model for your
application.

How to get images
-----------------

We obtain the images from http://wms.lroc.asu.edu/lroc/search (we search
for EDR images of type NACL and NACR).

A faster (but not as complete) interface is provided by
http://ode.rsl.wustl.edu/moon/indexproductsearch.aspx.
This site allows for bulk downloads, but does not permit datasets
bigger than several tens of GB, so several attempts may be necessary.

The related site http://ode.rsl.wustl.edu/moon/indextools.aspx?displaypage=lolardr 
can provide LOLA datasets which can be used as (sparse) ground truth.

If desired to use gridded LOLA DEMs, see :numref:`sfs-lola-dem`.

We advise the following strategy for picking images. First choose a
small longitude-latitude window in which to perform a search for
images. Pick two images that are very close in time and with a big
amount of overlap (ideally they would have consecutive orbit numbers).
Those can be passed to ASP's ``parallel_stereo`` tool to create an
initial DEM.  Then, search for other images close to the center of the
maximum overlap of the first two images. Pick one or more of those,
ideally with different illumination conditions than the first
two. Those (together with one of the first two images) can be used for
SfS.

To locate the area of spatial overlap, the images can be map-projected
(either with ``cam2map`` with a coarse resolution) or with
``mapproject``, using for example the LOLA DEM as the terrain to
project onto, or the DEM obtained from running ``parallel_stereo`` on
those images. Then the images can be overlayed as georeferenced images
in ``stereo_gui`` (:numref:`stereo_gui`). A good sanity check is to
examine the shadows in various images. If they point in different
directions in the images and perhaps also have different lengths, that
means that illumination conditions are different enough, which will
help constrain the ``sfs`` problem better.

An example for how to download and prepare the datasets is shown
in :numref:`sfs_single_image`.

.. _sfs_isis_vs_csm:

ISIS vs CSM models
------------------

CSM (:numref:`csm`) is a modern sensor model which can be used with
multiple threads, and can be on the order of 7-15 times faster than the
ISIS .cub model it is meant to replace, as benchmarked with
``mapproject``, ``bundle_adjust``, and ``sfs``.

Given a dataset of ISIS .cub camera files it is desired to run SfS on,
it is suggested to attempt to convert them to corresponding CSM models
as described in :numref:`create_csm_linescan`, and if the pixel errors as
output by ``cam_test`` are no more than the order of 0.5 pixels, to
use the CSM models instead of the ISIS ones in all the tools outlined
below (``parallel_bundle_adjust``, ``parallel_stereo``,
``mapproject``, and ``parallel_sfs``). The SfS DEMs obtained with
these two methods were observed to differ by several millimeters at
most, on average, but an evaluation may be necessary for your
particular case.

This will work only for the datasets of the original dimensions, so
not when the ``reduce`` or ``crop`` commands were used on the data.

Any of the commands further down which only use .cub files can be
adapted for use with CSM cameras by appending to those commands the
CSM .json cameras in the same order as the .cub files, from which only
the image information will then be used, with the camera information
coming from the .json files.

For example, if ``sfs`` is run with an ISIS camera as::

  sfs --use-approx-camera-models --crop-input-images \
    -i input_dem.tif image.cub -o sfs_isis/run

then, the corresponding command using the CSM model will be::

  sfs --crop-input-images                                \
    -i input_dem.tif image.cub image.json -o sfs_csm/run

The option ``--use-approx-camera-models`` is no longer necessary
as the CSM model is fast enough. It is however suggested to still
keep the ``--crop-input-images`` option. 

.. _sfs_single_image:

SfS at 1 meter/pixel using a single image
-----------------------------------------

In both this and the next sections we will work with LRO NAC images
taken close to the Lunar South Pole, at a latitude of 85 degrees
South (the tool was tested on equatorial regions as well). We will use
four images, M139939938LE, M139946735RE, M173004270LE, and M122270273LE.

Data preparation
^^^^^^^^^^^^^^^^

Fetch the images::

    wget http://lroc.sese.asu.edu/data/LRO-L-LROC-2-EDR-V1.0/LROLRC_0005/DATA/SCI/2010267/NAC/M139939938LE.IMG
    wget http://lroc.sese.asu.edu/data/LRO-L-LROC-2-EDR-V1.0/LROLRC_0005/DATA/SCI/2010267/NAC/M139946735RE.IMG
    wget http://lroc.sese.asu.edu/data/LRO-L-LROC-2-EDR-V1.0/LROLRC_0009/DATA/SCI/2011284/NAC/M173004270LE.IMG
    wget http://lroc.sese.asu.edu/data/LRO-L-LROC-2-EDR-V1.0/LROLRC_0002/DATA/MAP/2010062/NAC/M122270273LE.IMG

We convert these to ISIS cubes, initialize the SPICE kernels, and
perform radiometric calibration and echo correction. That is explained
in :numref:`lro_nac_no_stitch`.

It can be convenient to create preview versions of these datasets, at
10% of original resolution, for the purpose of inspection. That is
done as follows::

    f=M139939938LE
    reduce from = ${f}.cal.echo.cub to = ${f}.cal.echo.sub10.cub  \
      sscale = 10 lscale = 10

For simplicity, we create we create shorter aliases for these images::

    ln -s M139939938LE.cal.echo.cub A.cub
    ln -s M139946735RE.cal.echo.cub B.cub
    ln -s M173004270LE.cal.echo.cub C.cub
    ln -s M122270273LE.cal.echo.cub D.cub

.. _initial_sfs_dem:

Initial DEM creation
^^^^^^^^^^^^^^^^^^^^

The first step is to run stereo to create an initial guess DEM. We
picked for this the first two of these images. These form a stereo pair,
that is, they have a reasonable baseline and sufficiently close times of
acquisition (hence very similar illuminations). These conditions are
necessary to obtain a good stereo result.

::

    parallel_stereo --job-size-w 1024 --job-size-h 1024 A.cub B.cub \
      --left-image-crop-win 0 7998 2728 2696                        \
      --right-image-crop-win 0 9377 2733 2505                       \
      --threads 16 --corr-seed-mode 1  --subpixel-mode 3            \
      run_full1/run

See :numref:`running-stereo` for a discussion about various speed-vs-quality choices,
and :numref:`mapproj-example` about handling artifacts in steep terrain.
Consider using CSM cameras instead of ISIS cameras (:numref:`sfs_isis_vs_csm`).

Next we create a DEM at 1 meter/pixel, which is about the resolution of
the input images. We use the stereographic projection since this dataset
is very close to the South Pole.

::

    point2dem -r moon --stereographic --proj-lon 0           \
      --proj-lat -90 run_full1/run-PC.tif

SfS can only be run on a DEM with valid data at each grid point.  The
DEM obtained above should be opened in ``stereo_gui``, and the bounds
of a clip having only valid data should be determined
(:numref:`image_bounds`). Such a clip is cropped as::

    gdal_translate -projwin -15471.9 150986 -14986.7 150549  \
      run_full1/run-DEM.tif run_full1/run-crop-DEM.tif

This creates a DEM clip of size 456 |times| 410 pixels.  

The ``point2dem`` program auto-determines the DEM resolution (grid
size), as an estimate of the image *ground sample distance
(GSD)*. This is the optimal resolution to run SfS at. If creating your
input DEM some other way, it is strongly suggested to use a DEM grid
size not too different from the GSD, in order to get the best
quality. The ``mapproject`` program (:numref:`mapproject`), when
invoked with no input grid size, computes the grid size as the image
GSD, and that value can then be used when creating the input SfS DEM.

If this DEM has holes, those can be filled in ``dem_mosaic`` or with
``point2dem`` itself. The ``dem_mosaic`` tool can also apply some blur
to attenuate artifacts, though ``sfs`` has a smoothing term itself
which should take care of small imperfections in the input.

Running SfS
^^^^^^^^^^^

Then we run ``sfs`` on this clip (for a larger clip ``parallel_sfs``
should be used instead, see :numref:`parallel_sfs`)::

    sfs -i run_full1/run-crop-DEM.tif A.cub -o sfs_ref1/run           \
      --reflectance-type 1 --crop-input-images                        \
      --smoothness-weight 0.08 --initial-dem-constraint-weight 0.001  \
      --max-iterations 10 --use-approx-camera-models

The smoothness weight is a parameter that needs tuning. If it is too
small, SfS will return noisy results, if it is too large, too much
detail will be blurred. Here we used the Lunar-Lambertian model. The
meaning of the other ``sfs`` options can be looked up in :numref:`sfs`.

The value of ``--initial-dem-constraint-weight`` is best set to 0 when
the initial DEM is not very reliable, as otherwise defects from it can
be inherited by the SfS result. Otherwise a value between 0.0001 and
0.001 may be good enough.

See :numref:`sfs_albedo` for modeling of albedo. Shadow thresholds may
be needed to avoid artifacts in shadow. See
:numref:`sfs_crater_bottoms` for a potential solution to flat crater
bottoms in areas in shadow.

In the next sections, where SfS will be done with multiple images,
more parameters which can control the quality of the result will be
explored.

See :numref:`sfs_outputs` for where ``sfs`` stores its outputs.

Inspecting the results
^^^^^^^^^^^^^^^^^^^^^^

We show the results of running this program in :numref:`sfs1`. The
left-most figure is the hill-shaded original DEM, which was obtained
by running::

    hillshade --azimuth 300 --elevation 20 run_full1/run-crop-DEM.tif \
      -o run_full1/run-crop-hill.tif 

The second image is the hill-shaded DEM obtained after running ``sfs``
for 10 iterations.

The third image is, for comparison, the map-projection of A.cub onto the
original DEM, obtained via the command::

    mapproject --tr 1 run_full1/run-crop-DEM.tif A.cub A_map.tif \
      --tile-size 1024

(For small DEMs one can use a smaller ``--tile-size`` to start more
subprocesses in parallel to do the mapprojection. That is not needed
with CSM cameras as then ``mapproject`` is multithreaded.)

The fourth image is the colored absolute difference between the
original DEM and the SfS output, obtained by running ``geodiff``
(:numref:`geodiff`)::

    geodiff --absolute sfs_ref1/run-DEM-final.tif \
      run_full1/run-crop-DEM.tif -o out
    colormap --min 0 --max 2 out-diff.tif

.. figure:: images/sfs1.jpg
   :name: sfs1
   :alt: An sfs illustration 

   An illustration of ``sfs``. The images are, from left to right, the
   original hill-shaded DEM, the hill-shaded DEM obtained from ``sfs``,
   the image A.cub map-projected onto the original DEM, and the absolute
   difference of the original and final DEM, where the brightest shade
   of red corresponds to a 2 meter height difference.

It can be seen that the optimized DEM provides a wealth of detail and
looks quite similar to the input image. It also did not diverge
significantly from the input DEM. We will see in the next section that
SfS is in fact able to make the refined DEM more accurate than the
initial guess (as compared to some known ground truth), though that is
not guaranteed, and most likely did not happen here where just one image
was used.

.. _sfs_albedo:

Albedo modeling with one or more images
---------------------------------------

When using a single input image, it may be preferable to avoid floating
(solving for) the albedo (option ``--float-albedo``), hence to have it
set to 1 and kept fixed. The reason for that is because for a single
image it is not possible to distinguish if a bright image area comes
from lighter-colored terrain or from having an inclination which makes
it face the Sun more.

If desired to float the albedo with one image, it is suggested to use
a higher value of ``--initial-dem-constraint-weight`` to constrain the
terrain better in order to make albedo determination more reliable.
The albedo can be prevented from changing too much if the
``--albedo-constraint-weight`` parameter is used.

Albedo should be floated with two or more images, if albedo variations
are clearly visible, and if those images have sufficiently different
illumination conditions, as then the albedo and slope effects can be
separated more easily. For images not having obvious albedo variations
it may be prudent to keep the albedo fixed at the nominal value of 1.

It is important to use appropriate values for the
``--shadow-thresholds`` parameter, as otherwise regions in shadow will
be interpreted as lit terrain with a pitch-black color, and the computed
albedo and terrain will have artifacts.

See :numref:`sfs_outputs` for the location of the computed albedo.

SfS with multiple images in the presence of shadows
---------------------------------------------------

In this section we will run ``sfs`` with multiple images. We would
like to be able to see if SfS improves the accuracy of the DEM rather
than just adding detail to it. We evaluate this using the following
(admittedly imperfect) approach. We reduce the resolution of the
original images by a factor of 10, run stereo with them, followed by
SfS using the stereo result as an initial guess and with the resampled
images. As ground truth, we create a DEM from the original images at
the higher resolution of 1 meter/pixel, which we bring closer to the
initial guess for SfS using ``pc_align``. We would like to know if
running SfS brings us even closer to this "ground truth" DEM.

The most significant challenge in running SfS with multiple images is
that shape-from-shading is highly sensitive to errors in camera
position and orientation. It is suggested to bundle-adjust the cameras
first (:numref:`bundle_adjust`). 

It is important to note that bundle adjustment may fail if the images
have sufficiently different illumination, as it will not be able to
find matches among images. A solution to this is discussed in
:numref:`sfs-lola-comparison`, and it amounts to bridging the gap
between images with dis-similar illumination with more images of
intermediate illumination. 

It is strongly suggested that, when doing bundle adjustment, the
images should be specified in the order given by Sun azimuth angle
(see :numref:`sfs_azimuth`). The images should also be mapprojected
and visualized (in the same order), to verify that the illumination is
changing gradually.

To make bundle adjustment and stereo faster, we first crop the images,
such as shown below (the crop parameters can be determined via
``stereo_gui``, :numref:`stereo_gui`).

::

    crop from = A.cub to = A_crop.cub sample = 1 line = 6644 \
      nsamples = 2192 nlines = 4982
    crop from = B.cub to = B_crop.cub sample = 1 line = 7013 \
        nsamples = 2531 nlines = 7337
    crop from = C.cub to = C_crop.cub sample = 1 line = 1    \
      nsamples = 2531 nlines = 8305
    crop from = D.cub to = D_crop.cub sample = 1 line = 1    \
      nsamples = 2531 nlines = 2740

Note that manual cropping is not practical for a very large number of
images. In that case, it is suggested to mapproject the input images
onto some smooth DEM whose extent corresponds to the terrain to be
created with ``sfs`` (with some extra padding), then run bundle
adjustment with mapprojected images (option ``--mapprojected-data``,
illustrated in :numref:`sfs-lola-comparison`) and stereo also with
mapprojected images (:numref:`mapproj-example`). This will not only be
automated and faster, but also more accurate, as the inputs will be
more similar after mapprojection.

Bundle adjustment and stereo happens as follows::

    bundle_adjust A_crop.cub B_crop.cub C_crop.cub D_crop.cub \
      --num-iterations 100 --save-intermediate-cameras        \
      --ip-per-image 20000 --max-pairwise-matches 1000        \
      --min-matches 1 --num-passes 1 -o run_ba/run
    parallel_stereo A_crop.cub B_crop.cub run_full2/run       \
      --subpixel-mode 3 --bundle-adjust-prefix run_ba/run

One can try using the stereo option ``--nodata-value``
(:numref:`stereodefault`) to mask away shadowed regions, which may
result in more holes but less noise in the terrain created from
stereo.

See :numref:`running-stereo` for a discussion about various speed-vs-quality choices,
and :numref:`mapproj-example` about handling artifacts in steep terrain.
Consider using CSM cameras instead of ISIS cameras (:numref:`sfs_isis_vs_csm`).

The resulting cloud, ``run_full2/run-PC.tif``, will be used to create
the "ground truth" DEM. As mentioned before, we'll in fact run SfS
with images subsampled by a factor of 10. Subsampling is done by
running the ISIS ``reduce`` command::

    for f in A B C D; do 
      reduce from = ${f}_crop.cub to = ${f}_crop_sub10.cub \
        sscale = 10 lscale = 10
    done

We run bundle adjustment and parallel_stereo with the subsampled
images using commands analogous to the above. It was quite challenging
to find match points, hence the ``--mapprojected-data`` option in
``bundle_adjust`` was used, to find interest matches among
mapprojected images, so the process went as follows::

    # Prepare mapprojected images (see note in the text below)
    parallel_stereo A_crop_sub10.cub B_crop_sub10.cub \
      --subpixel-mode 3 run_sub10_noba/run
    point2dem -r moon --tr 10 --stereographic         \
      --proj-lon 0 --proj-lat -90                     \
      run_sub10_noba/run-PC.tif
    for f in A B C D; do 
      mapproject run_sub10_noba/run-DEM.tif --tr 10   \
        ${f}_crop_sub10.cub ${f}_sub10.map.noba.tif
    done

    # Run bundle adjustment
    bundle_adjust A_crop_sub10.cub B_crop_sub10.cub     \
      C_crop_sub10.cub D_crop_sub10.cub --min-matches 1 \
      --num-iterations 100 --save-intermediate-cameras  \
      -o run_ba_sub10/run --ip-per-image 20000          \
      --max-pairwise-matches 1000 --overlap-limit 50    \
      --match-first-to-last --num-passes 1              \
      --mapprojected-data                               \
      "$(ls [A-D]_sub10.map.noba.tif) run_sub10_noba/run-DEM.tif"
 
It is suggested to use above a DEM not much bigger than the eventual
area of interest, otherwise interest points which are far away may be
created. While that may provide robustness, in some occasions, given
that LRO NAC images are very long and can have jitter, interest points far
away could actually degrade the quality of eventual registration in 
the desired smaller area.

The option ``--max-pairwise-matches`` in ``bundle_adjust`` should
reduce the number of matches to the set value, if too many were
created originally.  The option ``--overlap-limit`` reduces the number
of subsequent images to be matched to the current one to this value.
 
Run stereo and create a DEM::

    parallel_stereo A_crop_sub10.cub B_crop_sub10.cub   \
      run_sub10/run --subpixel-mode 3                   \
     --bundle-adjust-prefix run_ba_sub10/run
     point2dem -r moon --tr 10 --stereographic          \
        --proj-lon 0 --proj-lat -90 run_sub10/run-PC.tif 

This will create a point cloud named ``run_sub10/run-PC.tif`` and
a DEM ``run_sub10/run-DEM.tif``.

It is strongly suggested to mapproject the bundle-adjusted images
onto this DEM and verify that the obtained images agree::

   for f in A B C D; do 
      mapproject run_sub10/run-DEM.tif               \
        ${f}_crop_sub10.cub ${f}_sub10.map.yesba.tif \
        --bundle-adjust-prefix run_ba_sub10/run
    done
    stereo_gui --use-georef --single-window *yesba.tif

We'll bring the "ground truth" point cloud closer to the initial
guess for SfS using ``pc_align``::

    pc_align --max-displacement 200 run_full2/run-PC.tif \
      run_sub10/run-PC.tif -o run_full2/run              \
      --save-inv-transformed-reference-points

This step is extremely important. Since we ran two bundle adjustment
steps, and both were without ground control points, the resulting
clouds may differ by a large translation, which we correct here. Hence
we would like to make the "ground truth" terrain aligned with the
datasets on which we will perform SfS.

Next we create the "ground truth" DEM from the aligned high-resolution
point cloud, and crop it to a desired region::

    point2dem -r moon --tr 10 --stereographic --proj-lon 0 --proj-lat -90 \
      run_full2/run-trans_reference.tif
    gdal_translate -projwin -15540.7 151403 -14554.5 150473               \
      run_full2/run-trans_reference-DEM.tif run_full2/run-crop-DEM.tif

We repeat the same steps for the initial guess for SfS::

    point2dem -r moon --tr 10 --stereographic --proj-lon 0 --proj-lat -90 \
      run_sub10/run-PC.tif
    gdal_translate -projwin -15540.7 151403 -14554.5 150473               \
      run_sub10/run-DEM.tif run_sub10/run-crop-DEM.tif

Since our dataset has many shadows, we found that specifying the
shadow thresholds for the tool improves the results. The thresholds
can be determined using ``stereo_gui``. This can be done by turning on
threshold mode from the GUI menu, and then clicking on a few points in
the shadows. The largest of the determined pixel values will be the
used as the shadow threshold. Then, the thresholded images can be
visualized/updated from the menu as well, and this process can be
iterated. See :numref:`thresh` for more details. We also found that
for LRO NAC a shadow threshold value of 0.003 works well enough
usually.

Alternatively, the ``otsu_threshold`` tool (:numref:`otsu_threshold`)
can be used to find the shadow thresholds automatically. It can
overestimate them somewhat.

Then, we run ``sfs``::

    sfs -i run_sub10/run-crop-DEM.tif A_crop_sub10.cub C_crop_sub10.cub \
      D_crop_sub10.cub -o sfs_sub10_ref1/run --threads 4                \
      --smoothness-weight 0.12 --initial-dem-constraint-weight 0.001    \
      --reflectance-type 1 --use-approx-camera-models                   \
      --max-iterations 5  --crop-input-images                           \
      --bundle-adjust-prefix run_ba_sub10/run                           \
      --blending-dist 10 --min-blend-size 20                            \
      --shadow-thresholds '0.00162484 0.0012166 0.000781663'

It is suggested to not vary the cameras with ``sfs`` (option
``--float-cameras``), as that should be done by bundle adjustment, and
``sfs`` will likely not arrive at a good solution for the cameras on
its own. Varying the exposures (option ``--float-exposure``) is likely
also unnecessary.

Note the two "blending" parameters, those help where there are seams
or light-shadow boundaries. The precise numbers may need
adjustment. In particular, decreasing ``--min-blend-size`` may result
in more seamless terrain models at the expense of some erosion.

One should experiment with floating the albedo (option
``--float-albedo``) if noticeable albedo variations are seen in the
images. See :numref:`sfs_albedo` for a longer discussion.

After this command finishes, we compare the initial guess to ``sfs`` to
the "ground truth" DEM obtained earlier and the same for the final
refined DEM using ``geodiff`` as in the previous section. Before SfS::

    geodiff --absolute run_full2/run-crop-DEM.tif \
      run_sub10/run-crop-DEM.tif -o out
    gdalinfo -stats out-diff.tif | grep Mean=  

and after SfS::

    geodiff --absolute run_full2/run-crop-DEM.tif \
      sfs_sub10_ref1/run-DEM-final.tif -o out
    gdalinfo -stats out-diff.tif | grep Mean=

The mean error goes from 2.64 m to 1.29 m, while the standard deviation
decreases from 2.50 m to 1.29 m. 

See :numref:`sfs2_fig` for an illustration. Visually the
refined DEM looks more detailed. The same
experiment can be repeated with the Lambertian reflectance model
(reflectance-type 0), and then it is seen that it performs a little
worse.

We also show in this figure the first of the images used for SfS,
``A_crop_sub10.cub``, map-projected upon the optimized DEM. Note that we
use the previously computed bundle-adjusted cameras when map-projecting,
otherwise the image will show as shifted from its true location::

    mapproject sfs_sub10_ref1/run-DEM-final.tif A_crop_sub10.cub   \
      A_crop_sub10_map.tif --bundle-adjust-prefix run_ba_sub10/run

.. _sfs_crater_bottoms:

Handling lack of data in shadowed crater bottoms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As seen in :numref:`sfs2_fig`, ``sfs`` makes the crater bottoms
flat in shadowed areas, where is no data. A fix for this is to add a
new curvature term in the areas in shadow, of the form

.. math::

   \label{curvature}
     w \left(\frac{\partial^2 \phi}{\partial x^2} + 
      \frac{\partial^2 \phi}{\partial y^2} - c\right)

to the SfS formulation in :numref:`sfs_formulation`. As an example, running::

    sfs -i run_sub10/run-crop-DEM.tif                               \
        A_crop_sub10.cub C_crop_sub10.cub D_crop_sub10.cub          \
        -o sfs_sub10_v2/run                                         \
        --threads 4 --smoothness-weight 0.12                        \
        --max-iterations 5 --initial-dem-constraint-weight 0.0001   \
        --reflectance-type 1                                        \
        --use-approx-camera-models                                  \
        --crop-input-images                                         \
        --bundle-adjust-prefix run_ba_sub10/run                     \
        --shadow-thresholds '0.002 0.002 0.002'                     \
        --curvature-in-shadow 0.15 --curvature-in-shadow-weight 0.1 \
        --lit-curvature-dist 10 --shadow-curvature-dist 5

will produce the terrain in :numref:`sfs2_fix_fig`.
 
The curvature ``c`` is given by option ``--curvature-in-shadow``, its
weight ``w`` by ``--curvature-in-shadow-weight``, and the parameters
``--lit-curvature-dist`` and ``--shadow-curvature-dist`` help gradually
phase in this term at the light-shadow interface, this many pixels
inside each corresponding region.

Some tuning of these parameters should be done depending on the
resolution.

.. _sfs2_fig:
.. figure:: images/sfs2.jpg
   :name: sfs2_fig_name
   :alt: Another sfs illustration 

   An illustration of ``sfs``. The images are, from left to right, the
   hill-shaded initial guess DEM for SfS, the hill-shaded DEM obtained
   from ``sfs``, the "ground truth" DEM, and the first of the images
   used in SfS map-projected onto the optimized DEM.

.. _sfs2_fix_fig:
.. figure:: images/sfs2_fix_depth.jpg
   :name: sfs2_fix_fig_name
   :alt: SfS with curved crater bottom

   An illustration of adding a curvature term to the SfS cost
   function, per :numref:`sfs_crater_bottoms`. It can be seen that,
   compared to the earlier figure, the crater bottom is now curved,
   rather than flat, but more modeling is needed to ensure a seamless
   transition.

.. _sfs-lola-comparison:

SfS with big illumination changes and comparison with LOLA
----------------------------------------------------------

SfS is very sensitive to errors in camera positions and orientations.
As discussed earlier, bundle adjustment should be used to correct
these errors, and if the images have different enough illumination
that bundle adjustment fails, one should add new images with
intermediate illumination conditions (while sorting the full set of
images by Sun azimuth angle (:numref:`sfs_azimuth`) and verifying
visually by mapprojection the gradual change in illumination, as
described in :numref:`sfs-lola-dem`).

As a fallback alternative, interest point matches among the images can
be selected manually. Picking about 4 interest pints in each image may
be sufficient. Ideally they should be positioned far from each other,
to improve the accuracy.

Below is one example of how we select interest points, run SfS, and then
compare to LOLA, which is an independently acquired sparse dataset of 3D
points on the Moon. According to :cite:`smith2011results`,
the LOLA accuracy is on the order of 1 m. To ensure a meaningful
comparison of stereo and SfS with LOLA, we resample the LRO NAC images
by a factor of 4, making them nominally 4 m/pixel. This is not strictly
necessary, the same exercise can be repeated with the original images,
but it is easier to see the improvement due to SfS when comparing to
LOLA when the images are coarser than the LOLA error itself.

.. _sfs_initial_terrain:

Initial terrain creation
^^^^^^^^^^^^^^^^^^^^^^^^

We work with the same images as before. They are resampled as follows::

    for f in A B C D; do 
      reduce from = ${f}_crop.cub to = ${f}_crop_sub4.cub sscale=4 lscale=4
    done

The ``parallel_stereo`` and ``point2dem`` tools are run to get a first-cut DEM.
Bundle adjustment is not done at this stage yet::

    parallel_stereo A_crop_sub4.cub B_crop_sub4.cub                  \
      run_stereo_noba_sub4/run --subpixel-mode 3
    point2dem --stereographic --proj-lon -5.7113 --proj-lat -85.0003 \
      run_stereo_noba_sub4/run-PC.tif --tr 4 

One can try using the stereo option ``--nodata-value``
(:numref:`stereodefault`) to mask away shadowed regions, which may
result in more holes but less noise in the terrain created from
stereo.

See :numref:`running-stereo` for a discussion about various
speed-vs-quality choices, and :numref:`mapproj-example` about handling
artifacts in steep terrain.  Consider using CSM cameras instead of
ISIS cameras (:numref:`sfs_isis_vs_csm`).

To find stereo pairs, one can bundle-adjust a handful of images together
(sorted by illumination), then inspect the produced file::

    {output-prefix}-convergence_angles.txt

See :numref:`ba_out_files` and :numref:`stereo_pairs` for more details
and criteria for picking a stereo pair.

We would like now to automatically or manually pick interest points
for the purpose of doing bundle adjustment. It much easier to compute
these if the images are first mapprojected, which brings them all
into the same perspective. This approach is described in :numref:`mapip`,
and here just the relevant commands are shown.

::

    for f in A B C D; do 
      mapproject --tr 4 run_stereo_noba_sub4/run-DEM.tif \
        ${f}_crop_sub4.cub ${f}_crop_sub4.noba.tif       \
        --tile-size 1024
    done

Optional manual interest point picking in the mapprojected images can
happen here. Those should be saved using the output prefix expected
below.  Here mapprojection was used without
``--bundle-adjust-prefix``. Here it is not important that the
mapprojected images are misaligned, as after match points are found
these points will be projected back to camera pixel space.

::

    mapped_images=$(echo {A,B,C,D}_crop_sub4.noba.tif)
    dem=run_stereo_noba_sub4/run-DEM.tif
    bundle_adjust A_crop_sub4.cub B_crop_sub4.cub      \
      C_crop_sub4.cub D_crop_sub4.cub                  \
      --mapprojected-data "$mapped_images $dem"        \
      --num-iterations 100 --save-intermediate-cameras \
      --min-matches 1 --max-pairwise-matches 1000      \
      --ip-per-image 20000 --overlap-limit 50          \
      --match-first-to-last --num-passes 1             \
      -o run_ba_sub4/run

An illustration is shown in :numref:`sfs3`.

.. figure:: images/sfs3.jpg
   :name: sfs3
   :alt: interest points picked manually

   An illustration of how interest points are picked manually for the
   purpose of bundle adjustment and then SfS. This is normally not necessary
   if there exist images with intermediate illumination.

If in doubt, it is suggested that more points be picked, and one should
examine the resulting reprojection errors in the final ``pointmap`` file
(:numref:`ba_out_files`).

Note that if several attempts are used to pick and save interest
points in the mapprojected images, the resulting match file among the
unprojected images needs to be deleted before re-running bundle
adjustment, as otherwise it won't be recreated.

Then we run ``parallel_stereo`` with the adjusted cameras::

    parallel_stereo A_crop_sub4.cub B_crop_sub4.cub                    \
      run_stereo_yesba_sub4/run --subpixel-mode 3                      \
      --bundle-adjust-prefix run_ba_sub4/run
    point2dem --stereographic --proj-lon -5.7113 --proj-lat -85.0003   \
      run_stereo_yesba_sub4/run-PC.tif --tr 4

Mapproject the bundle-adjusted images onto the stereo terrain obtained
with bundle-adjusted images::

    for f in A B C D; do 
      mapproject --tr 4 run_stereo_yesba_sub4/run-DEM.tif      \
        ${f}_crop_sub4.cub ${f}_crop_sub4.ba.tif               \
        --tile-size 1024 --bundle-adjust-prefix run_ba_sub4/run
    done

A good sanity check is to overlay this DEM and the resulting
mapprojected images and check for registration errors.

This will also show where the images overlap, and hence on what
portion of the DEM we can run SfS. We select a clip for that as
follows::

    gdal_translate -srcwin 138 347 273 506   \
      run_stereo_yesba_sub4/run-DEM.tif      \
      run_stereo_yesba_sub4/run-crop-DEM.tif 

SfS and alignment in current DEM's coordinate system
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We would like to compare the DEM clip, before and after SfS, to measured 
LOLA data. For that, an alignment of the two is first necessary. 
Here we will bring the LOLA data in DEM's coordinate system, as that seems
simpler, and later we will discuss the reverse approach.
 
Run SfS::

    sfs -i run_stereo_yesba_sub4/run-crop-DEM.tif A_crop_sub4.cub \
      C_crop_sub4.cub D_crop_sub4.cub                             \
      -o sfs_sub4_ref1_th_reg0.12_wt0.001/run                     \
      --shadow-thresholds '0.00149055 0.00138248 0.000747531'     \
      --threads 4 --smoothness-weight 0.12                        \
      --initial-dem-constraint-weight 0.001 --reflectance-type 1  \
      --max-iterations 10 --use-approx-camera-models              \
      --blending-dist 10 --min-blend-size 30                      \
      --crop-input-images --bundle-adjust-prefix run_ba_sub4/run

One should experiment with floating the albedo (option
``--float-albedo``) if noticeable albedo variations are seen in the
images. See :numref:`sfs_albedo` for a longer discussion.

To get more seamless results around small shadowed craters reduce the
value of ``--min-blend-size``. If you have many such craters very
close to each other, this may result in some erosion, however.

Fetch the portion of the LOLA dataset around the current DEM from the
site described earlier, and save it as
``RDR_354E355E_85p5S84SPointPerRow_csv_table.csv``. Bring the LOLA
dataset into the coordinate system of the DEM::

    pc_align --max-displacement 280 run_stereo_yesba_sub4/run-DEM.tif \
      RDR_354E355E_85p5S84SPointPerRow_csv_table.csv                  \
      -o run_stereo_yesba_sub4/run --save-transformed-source-points

Then we compare to the aligned LOLA dataset the input to SfS and its
output::

    geodiff --absolute -o beg --csv-format '1:lon 2:lat 3:radius_km' \
      run_stereo_yesba_sub4/run-crop-DEM.tif                        \
      run_stereo_yesba_sub4/run-trans_source.csv
    geodiff --absolute -o end --csv-format '1:lon 2:lat 3:radius_km' \
      sfs_sub4_ref1_th_reg0.12_wt0.001/run-DEM-final.tif             \
      run_stereo_yesba_sub4/run-trans_source.csv

We see that the mean error between the DEM and LOLA goes down, after
SfS, from 1.14 m to 0.90 m, while the standard deviation decreases from
1.18 m to 1.06 m.

.. _sfs-move-cameras:

SfS and alignment in LOLA's coordinates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The alternative to the approach above is to work in the LOLA coordinate
system. This requires transforming the DEM clip (and later the cameras)::

    pc_align --max-displacement 280                  \
      run_stereo_yesba_sub4/run-crop-DEM.tif         \
      RDR_354E355E_85p5S84SPointPerRow_csv_table.csv \
      --save-inv-transformed-reference-points        \
      -o run_align/run 

The 50th *error percentile of smallest errors* should be under
1-2 meters, and ideally less. Otherwise likely something is not right,
and coregistration of images may fail later.

The resulting transformed cloud needs to be regridded::

    point2dem --stereographic --proj-lon -5.7113 --proj-lat -85.0003 \
      run_align/run-trans_reference.tif --tr 4
 
obtaining ``run_align/run-trans_reference-DEM.tif``. This DEM should
be hillshaded and overlayed on top of the LOLA DEM and see if there is
any noticeable shift, which would be a sign of alignment not being
successful. The ``geodiff`` tool can be used to examine any
discrepancy among the two (:numref:`geodiff`), followed by
``colormap`` (:numref:`colormap`) and inspection in ``stereo_gui``.

The cameras can be moved with ``bundle_adjust``::

    bundle_adjust --input-adjustments-prefix run_ba_sub4/run  \
      --initial-transform run_align/run-inverse-transform.txt \
      --apply-initial-transform-only                          \
      -o ba_align/run

It is very important to note that we used above
``run-inverse-transform.txt``, which goes from the DEM coordinate
system to the LOLA one. This is discussed in detail in
:numref:`ba_pc_align`.

It is suggested to mapproject the images using the obtained
bundle-adjusted cameras onto the obtained DEM, and check for alignment
errors. Large ones means that something went wrong. Either not enough
iterations took place, or there are not enough matches among some
images.

Very small alignment errors may be reduced by invoking
``bundle_adjust`` one more time, using latest cameras in
``ba_align/run``, with this aligned DEM as a constraint, and the
option ``--heights-from-dem``, as discussed in :numref:`sfs-lola-dem`.

SfS is done as above, while changing the initial terrain and the
cameras to use the newly aligned versions::

    sfs -i run_align/run-trans_reference-DEM.tif                 \
      A_crop_sub4.cub C_crop_sub4.cub D_crop_sub4.cub            \
      -o sfs_align_sub4_ref1_th_reg0.12_wt0.001/run              \
      --shadow-thresholds '0.00149055 0.00138248 0.000747531'    \
      --threads 4 --smoothness-weight 0.12                       \
      --initial-dem-constraint-weight 0.001 --reflectance-type 1 \
      --max-iterations 10 --use-approx-camera-models             \
      --blending-dist 10 --min-blend-size 30                     \
      --crop-input-images --bundle-adjust-prefix ba_align/run

See more comments about some of these parameters at the previous
invocation of ``sfs`` higher up.

The ``geodiff`` tool can then be used to compare the obtained SfS DEM
with the original LOLA dataset. Care is needed to populate correctly
the ``--csv-format`` option of ``geodiff``.

.. _sfs-lola-dem:

Running SfS with an external initial guess DEM and extreme illumination
-----------------------------------------------------------------------

Challenges
^^^^^^^^^^

Here we will illustrate how SfS can be run in a very difficult
situation. 

It is very strongly suggested to follow the instructions below very
carefully, and be very meticulous in doing various checks along the
way, as suggested. Otherwise one is looking at spending a lot of time
and producing suboptimal results. It is very hard to figure out
what the problems are by tracing one's work backward, hence, one
should proceed step-by-step and very carefully.

We chose a site very close to the Lunar South Pole, at around
89.7 degrees South. We used an external DEM as an initial guess
terrain, in this case the LOLA gridded DEM, as such a DEM has values in
permanently shadowed regions. The terrain size is 5 km by 5 km at 1
meter/pixel (we also ran a 10 km by 10 km region in the same location).

A difficulty here is that the topography is very steep, the
shadows are very long and vary drastically from image to image, and
some portions of the terrain show up only in some images. All this
makes it very hard to register images to each other and to the
ground. We solved this by choosing very carefully a large set of
representative images with gradually varying illumination.

We recommend that the process outlined below first be practiced with
just a couple of images on a small region, which will make it much
faster to iron out any issues.

Initial LOLA terrain
^^^^^^^^^^^^^^^^^^^^

The first step is to fetch the underlying LOLA DEM. We use the 20
meter/pixel one, resampled to 1 meter/pixel, creating a DEM named
``ref.tif``.

::

    wget http://imbrium.mit.edu/DATA/LOLA_GDR/POLAR/IMG/LDEM_80S_20M.IMG
    wget http://imbrium.mit.edu/DATA/LOLA_GDR/POLAR/IMG/LDEM_80S_20M.LBL
    pds2isis from = LDEM_80S_20M.LBL to = ldem_80s_20m.cub
    image_calc -c "0.5*var_0" ldem_80s_20m.cub -o ldem_80s_20m_scale.tif
    gdalwarp -overwrite -r cubicspline -tr 1 1 -co COMPRESSION=LZW \
      -co TILED=yes -co INTERLEAVE=BAND                            \
      -co BLOCKXSIZE=256 -co BLOCKYSIZE=256                        \
      -te -7050.5 -10890.5 -1919.5 -5759.5                         \
      ldem_80s_20m_scale.tif ref.tif

Note that we scaled its heights by 0.5 per the information in the LBL
file. The documentation of your DEM needs to be carefully studied to
see if this applies in your case. 

The site::

    https://core2.gsfc.nasa.gov/PGDA/LOLA_5mpp/

has higher-accuracy LOLA DEMs but only for a few locations.

Later when we mapproject images onto this DEM at 1 m/pixel, those will
be computed at integer multiples of this grid. Given that the grid
size is 1 m, the extent of those images as displayed by ``gdalinfo``
will have a fractional value of 0.5. It is very recommended to have
``gdalwarp`` above produce a result on the same grid (for when
``sfs_blend`` is used later). Hence, ``gdalwarp`` was used
with the ``-te`` option, with the bounds having a fractional part of 0.5.
Note that the bounds passed to ``-te`` are in the order::

    xmin, ymin, xmax, ymax

The ``dem_mosaic`` program can be used to automatically compute the bounds
of a DEM or orthoimage and change them to integer multiples at pixel size. It
can be invoked, for example, as::

    dem_mosaic --tr 1 --tap input.tif -o output.tif

The DEM grid size should be not too different from the *ground sample
distance (GSD)* of the images, for optimal results. That one can be found
with ``mapproject`` (:numref:`mapproject`).

Above the interpolated DEM was created with bicubic spline
interpolation, which is preferable to the default nearest neighbor
interpolation, and it was saved internally using blocks of size 256 x
256, which ASP handles better than the GDAL default with each block
as tall or wide as a row or column.

.. _sfs_azimuth:

Image selection and sorting by illumination
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

By far the hardest part of this exercise is choosing the images. 
We downloaded several hundred of them by visiting the web site noted
earlier and searching by the longitude-latitude bounds. The .IMG images
were converted to .cub as before, and they were mapprojected onto the
reference DEM, initially at a lower resolution to get a preview of
things.

The mapprojected images were mosaicked together using ``dem_mosaic``
with the option ``--block-max``, with a large value of ``--block-size``
(larger than the image size), and using the ``--t_projwin`` option to
specify the region of interest (in ``stereo_gui`` one can find this
region by selecting it with Control-Mouse). When the mosaicking tool
runs, the sum of pixels in the current region for each image will be
printed to the screen. Images with a positive sum of pixels are likely
to contribute to the desired region. Example::

   dem_mosaic --block-max --block-size 10000 --threads 1   \
     --t_projwin -7050.500 -10890.500 -1919.500 -5759.500  \
     M*.map.lowres.tif -o tmp.tif | tee pixel_sum_list.txt

The obtained subset of images should be sorted by illumination
conditions, that is, the Sun azimuth. This angle is printed when
running ``sfs`` with the ``--query`` option on the .cub files. Out of
those, the following were kept::

    M114859732RE.cal.echo.cub       73.1771
    M148012947LE.cal.echo.cub       75.9232
    M147992619RE.cal.echo.cub       78.7806
    M152979020RE.cal.echo.cub       96.895
    M117241732LE.cal.echo.cub       97.9219
    M152924707RE.cal.echo.cub       104.529
    M150366876RE.cal.echo.cub       104.626
    M152897611RE.cal.echo.cub       108.337
    M152856903RE.cal.echo.cub       114.057
    M140021445LE.cal.echo.cub       121.838
    M157843789LE.cal.echo.cub       130.831
    M157830228LE.cal.echo.cub       132.74
    M157830228RE.cal.echo.cub       132.74
    M157809893RE.cal.echo.cub       135.604
    M139743255RE.cal.echo.cub       161.014
    M139729686RE.cal.echo.cub       162.926
    M139709342LE.cal.echo.cub       165.791
    M139695762LE.cal.echo.cub       167.704
    M142240314RE.cal.echo.cub       168.682
    M142226765RE.cal.echo.cub       170.588
    M142213197LE.cal.echo.cub       172.497
    M132001536LE.cal.echo.cub       175.515
    M103870068LE.cal.echo.cub       183.501
    M103841430LE.cal.echo.cub       187.544
    M142104686LE.cal.echo.cub       187.765
    M162499044LE.cal.echo.cub       192.747
    M162492261LE.cal.echo.cub       193.704
    M162485477LE.cal.echo.cub       194.662
    M162478694LE.cal.echo.cub       195.62
    M103776992RE.cal.echo.cub       196.643
    M103776992LE.cal.echo.cub       196.643

(the Sun azimuth is shown on the right, in degrees). These were then
mapprojected onto the reference DEM at 1 m/pixel using the South Pole
stereographic projection specified in this DEM.

It is best to avoid images with very low illumination angles as those
can result in artifacts in the produced SfS terrain.

The primary reason why registration may fail later is illumination
varying too drastically from image to next images in the list,
and not being able to find matching interest points. Hence, there must
be sufficient images so illumination conditions change slowly as
one goes down the list.

Consider using here CSM models instead of ISIS models, as mentioned in
:numref:`sfs_isis_vs_csm`.

.. _sfs_exclude_lowres:

Excluding low-resolution images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It may be possible that images with low resolution may affect the
overall registration accuracy. Use a command as::

  mapproject --query-projection dem.tif image.cub image.json out.tif

to identify an image's ground sample distance (pixel size on the
ground). 

For LRO NAC, ideally it is suggested to exclude all images where this
value is above 1.5 meters. However, note that excluding images may
make it hard to match images with different illumination conditions,
as valuable images with intermediate illumination may be taken out. In
that case, one may want to keep low-resolution images for registration
but exclude them from shape-from-shading.

Handling a very large number of images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the chosen site is large, one may be looking at perhaps 1500 to
2000 images in it, and then it becomes quite hard to choose which
images to use. Two approaches can help with that.

One is to break up the input DEM in four quadrants with overlap.
Examine all images overlapping with a quadrant (after mapprojection at
a low resolution), sort them by Sun azimuth, and choose a subset of
good images for that quadrant. Repeat for the other quadrants. (Some
images may be repeated across quadrants, which is a good thing, as
eventually all these will be bundle-adjusted together; repeated images
should be removed before bundle adjustment.)

Alternatively, divide the images into groups, by Sun azimuth angle,
for example into 36 groups with variation of at most 10 degrees within
each group. Mapproject these as before, examine each group
individually, such as by opening them with::

  stereo_gui --hide-all --single-window --use-georef $(cat group1.txt)

and select a subset of them which cover reasonably well the desired
area, while eliminating those which don't seem to provide new
information. One should avoid the temptation of using the images with
the biggest footprint, as those may also be the ones at lower
resolution.

Note that clicking on an image in ``stereo_gui`` will print the image
name in the terminal, and that way a list can be assembled.

These two strategies can be used together, and they may be
automated at some point.

It is strongly suggested to carefully reduce the number of images to
at most 300 or so, with good overlap among them and representative and
gradually changing illumination, as otherwise the problem becomes
quickly unmanageable.  That may require working on a site which is
perhaps no more than 5000 pixels on the side. Note that removing
images with intermediate illumination can make bundle adjustment fail
to produce accurate results.

The complexity of the problem and the amount of work becomes very high
as the number of images and dimensions of the site goes up, and as
the illumination becomes more difficult.

Bundle adjustment (registration)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``parallel_bundle_adjust`` tool is employed to co-register the images
and correct camera errors. The images should be, as mentioned earlier,
ordered by Sun azimuth angle.

::

    parallel_bundle_adjust --processes 4 --ip-per-image 20000 \
      --overlap-limit 50 --num-iterations 1000 --num-passes 1 \
      --min-matches 1 --max-pairwise-matches 1000             \
      --camera-weight 0 --robust-threshold 5                  \ 
      --remove-outliers-params "75.0 3.0 100 100"             \
      --nodes-list <list of computing nodes>                  \
      --datum D_MOON <images> <cameras>                       \
      --mapprojected-data "<mapprojected images> ref.tif"     \
      --save-intermediate-cameras --match-first-to-last       \
      --min-triangulation-angle 0.1 -o ba/run 

For bundle adjustment we in fact used even more images that overlap
with this area, but likely this set is sufficient, and it is this set
that was used later for shape-from-shading (see also the
``--auto-overlap-limit`` option, which can be used to determine which
images overlap). Here more bundle adjustment iterations are desirable,
but this step takes too long. A large ``--ip-per-image`` can make a
difference in images with rather different different illumination
conditions but it can also slow down the process a lot. Note that the
value of ``--max-pairwise-matches`` was set to 1000. That should
hopefully create enough matches among any two images. A higher value
here will make bundle adjustment run slower and use more memory.

It is very important to have a lot of images during bundle adjustment,
and that they are sorted by illumination (Sun azimuth angle) to ensure that
there are enough overlaps and sufficiently similar illumination
conditions among them for bundle adjustment to succeed. Later, just a
subset can be used for shape-from-shading, enough to cover the entire
region, preferable with multiple illumination conditions at each
location.

Towards the poles the Sun may describe a full loop in the sky, and
hence the earliest images (sorted by Sun azimuth angle) may become
similar to the latest ones. That is the reason above we used the
option ``--match-first-to-last``.

The option ``--mapprojected-data`` is needed when the interest point
matches are hard to find. See See :numref:`mapip` for more details.

Note that this invocation may run for more than a day, or even
more. And it may be necessary to get good convergence. If the process
gets interrupted, or the user gives up on waiting, the adjustments
obtained so far can still be usable, if invoking bundle adjustment,
as above, with ``--save-intermediate-cameras``. As before, using
the CSM model can result in much-improved performance. 

Here we used ``--camera-weight 0`` and ``--robust-threshold 5`` to
give cameras which start far from the solution more chances to
converge. We used ``--num-passes 1`` in case 100 iterations may not be
enough to converge fully, and then outliers will be removed
prematurely if there is more than one pass. That would remove along
the way good interest point matches, which may make the solution
worse. The option ``--remove-outliers-params`` above also used very
generous outlier filtering, if more than 1 pass is desired, when
outlier filter takes place.

If there are on the order of 1000 input images, the underlying GNU
Parallel program may fail. In that case use the ``--image-list``,
``--camera-list`` and ``--mapprojected-data-list`` options
(:numref:`bundle_adjust`).

The file::

   ba/run-final_residuals_stats.txt

should also be examined. If many cameras have large median
reprojection error, that may be a sign of issues. Normally the option
``--robust-threshold 5`` should be good enough at making the camera
poses converge. One should consider removing cameras with the biggest
errors and too few interest point matches at this stage.

Alignment to ground
^^^^^^^^^^^^^^^^^^^

A very critical part of the process is to move from the coordinate
system of the cameras to the coordinate system of the ground in
``ref.tif``. For that, we perform an alignment transform from the
sparse cloud::

    ba/run-final_residuals_pointmap.csv  

in camera coordinates to ``ref.tif``::
 
    pc_align --max-displacement 400 --save-transformed-source-points \
      --compute-translation-only                                     \
      --csv-format '1:lon 2:lat 3:height_above_datum'                \
      ref.tif ba/run-final_residuals_pointmap.csv                    \
      -o ba/run 

This operation is rather fragile, and the resulting alignment may not
be sufficiently precise. If among the input images there exists a
stereo pair, it is suggested to instead align ``ref.tif`` to the DEM
obtained from that stereo pair, then use that alignment to transform
the cameras to the coordinate system of ``ref.tif``, before continuing
with SfS, as shown in :numref:`sfs-move-cameras`.

The value of ``--max-displacement`` could be too high perhaps, it is
suggested to also experiment with half of that and keep the result that
has the smaller error.

Note that earlier, in bundle adjustment, the option
``--min-triangulation-angle 0.1`` was used. If in doubt, that value
can be increased, perhaps to 0.5 degrees. The effect will be to remove
from the file ``residuals_pointmap.csv`` somewhat unreliable
triangulated points obtained from rays which are too close to being
parallel. This may improve the reliability of the alignment above,
but there is the risk that too many points may be removed. A solid
value for this min triangulation angle is 5-10 degrees, but LRO NAC
images are very rarely acquired with such a big difference in
perspective, and then this will remove a majoirty of interest points.

The flag ``--compute-translation-only`` turned out to be necessary as
``pc_align`` was introducing a bogus rotation.

The obtained alignment transform can be applied to the cameras to make
them aligned to the ground in ``ref.tif``::

    mkdir -p ba_align
    bundle_adjust --initial-transform ba/run-transform.txt       \
      --apply-initial-transform-only                             \
      --input-adjustments-prefix ba/run <images> -o ba_align/run

Since ``ref.tif`` was the first argument to ``pc_align``, above we
applied the transform ``ba/run-transform.txt`` which goes from the
coordinate system of cameras to the one of ``ref.tif``. If
``pc_align`` was invoked with the clouds in reverse order, for some
reason, then this transform would go from ``ref.tif`` to camera
coordinates, so to bring the cameras in the coordinates of ``ref.tif``
one would then apply the transform in
``ba/run-inverse-transform.txt``. See also :numref:`ba_pc_align`.

The images should now be projected onto this DEM as::

    mapproject --tr 1 --bundle-adjust-prefix ba_align/run \
      ref.tif image.cub image.map.tif

One should verify if they are precisely on top of each other and on
top of the LOLA DEM in ``stereo_gui`` :numref:`stereo_gui`). If any
shifts are noticed, with the images relative to each other, or to this
DEM, that is a sign of some issues. If the shift is relative to this
DEM, perhaps one can try the alignment above with a different value of
the max displacement.

Alignment using a stereo terrain
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The above alignment may not always be successful, since, if all the
cameras have small convergence angles, the ``residuals_pointmap.csv``
file may not have accurate 3D positions. If a stereo pair exists among
the bundle-adjusted images, it is strongly suggested to create a DEM from
that one and use it for alignment to the reference DEM
(:numref:`sfs-move-cameras`).

At the end of bundle adjustment the convergence angles for each pair
of images having matches are saved to disk (:numref:`ba_out_files`), 
to::

    {output-prefix}-convergence_angles.txt

That list can be used to uncover stereo pairs (the convergence angle
for a reliable stereo pair should be no less than 10 degrees,
:numref:`stereo_pairs`).
 
Registration refinement
^^^^^^^^^^^^^^^^^^^^^^^

If the images mapproject reasonably well onto the given DEM, with no
shift across the board, but there are still some registration errors,
one can refine the cameras using the reference terrain as a constraint
in bundle adjustment (:numref:`heights_from_dem`)::

    bundle_adjust --skip-matching --num-iterations 500  \
      --num-passes 1 --camera-weight 0                  \
      --input-adjustments-prefix ba_align/run <images>  \
      --save-intermediate-cameras                       \
      --heights-from-dem ref.tif                        \
      --heights-from-dem-weight 1.0                     \
      --heights-from-dem-robust-threshold 0.25          \
      --remove-outliers-params "75.0 3.0 100 100"       \
      --match-first-to-last --max-pairwise-matches 1000 \
      --match-files-prefix ba/run -o ba_align_ref/run

Note how we use the match files from the original ``ba`` directory,
and also use ``--skip-matching`` to save time by not recomputing
them. But the camera adjustments come from ``ba_align``, as the ones
in ``ba`` are before alignment. 

If 500 iterations turns out to take more than a day, it is suggested
to switch to CSM cameras (:numref:`sfs_isis_vs_csm`). It is suggested
that the images be map-projected with the cameras both before and
after this step, and see if things improve. If this procedure resulted
in improved but imperfect alignment, it may be run second time using
the new cameras as initial guess (and reusing the match files, etc.,
as before).

The switch ``--save-intermediate cameras`` is helpful, as before, if
desired to stop if things take too long.

After mapprojecting with the newly refined cameras in
``ba_align_ref``, any residual alignment errors should go away. 

The value used for ``--heights-from-dem-weight`` may need some
experimentation. Making it too high may result in a tight coupling to
the reference DEM at the expense of self-consistency between the
cameras. Yet making it too low may not constrain sufficiently the
uncertainty that exists in the height of triangulated points after
bundle adjustment, which is rather high since LRO NAC is mostly
looking down so the convergence angle among any rays going through
matching interest points is small.

It is suggested that the user examine the file::

    ba_align_ref/run-final_residuals_pointmap.csv

to see if the reprojection errors (column 4) are reasonably small, say
mostly on the order of 0.1 pixels (some outliers are expected
though). The triangulated point cloud from this file should also
hopefully be close to the reference DEM. Their difference is found
as::

    geodiff --absolute                                         \
      --csv-format '1:lon 2:lat 3:height_above_datum' ref.tif  \
      ba_align_ref/run-final_residuals_pointmap.csv            \
      -o ba_align_ref/run

Some of the differences that will be saved to disk are likely outliers,
but mostly they should be small, perhaps on the order of 1 meter.

The file::

   ba_align_ref/run-final_residuals_stats.txt

should also be examined. For each camera it has the median of the
norms of all residuals (reprojection errors) of pixels
projecting in that camera. Images for which this median is larger than
1 pixel or which have too few such residuals (see the ``count`` field
in that file) should be excluded from running SfS, as likely for those
cameras are not correctly positioned.

If, even after this step, the mapprojected images fail to be perfectly
on top of each other, or areas with poor coverage exist, more images
with intermediate illumination conditions and more terrain coverage
should be added and the process should be restarted. As a last resort,
any images that do not overlay correctly must be removed from
consideration for the shape-from-shading step.

Handling remaining registration errors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the results are not good so far, it could be because images
were not sorted by illumination for bundle adjustment, or the illumination
changed too drastically for an image in the list and all images that
follow it, or there are not enough overlaps between and image and the rest.

it is suggested to perhaps narrow down the domain of computation, by
cropping the input DEM to a region of perhaps 5000 pixels on the side,
ensure the images are sorted by illumination, align them using a
stereo pair (:numref:`sfs-move-cameras`), select a smaller subset of
perhaps 100-200 images, then redo bundle adjustment with the options
``--proj-win`` and ``--proj-str`` to exclude interest points outside
of given area.

The previously obtained camera adjustments can be used as initial
guesses when re-running bundle adjustment. 

If still no luck, mapproject the images using the latest cameras, open
them in ``stereo_gui`` while sorted by illumination, and carefully
inspect them. If a problematic image pair was found, see if it has
interest point matches in the earliest bundle adjustment directory.

If good results are obtained for such small subregions, the hope is
that a new bundle adjustment which uses the union of the individually
validated subsets (and using as input adjustments computed so far)
will result in a good solution.

If the option ``--mapproj-dem`` (with a DEM file as a value) is
specified, a report will be written having measurements regarding how
well images with bundle-adjusted cameras agree after mapprojection
(:numref:`ba_out_files`). This is very helpful in identifying
misregistered images.

Running SfS
^^^^^^^^^^^

Next, SfS follows::

    parallel_sfs -i ref.tif <images> --shadow-threshold 0.005        \
      --bundle-adjust-prefix ba_align_ref/run -o sfs/run             \ 
      --use-approx-camera-models --crop-input-images                 \
      --blending-dist 10 --min-blend-size 50 --threads 4             \
      --smoothness-weight 0.08 --initial-dem-constraint-weight 0.001 \
      --reflectance-type 1 --max-iterations 5  --save-sparingly      \
      --tile-size 200 --padding 50 --processes 20                    \
      --nodes-list <machine list>

It was found empirically that a shadow threshold of 0.005 was good
enough.  It is also possible to specify individual shadow thresholds
if desired, via ``--custom-shadow-threshold-list``. This may be useful
for images having diffuse shadows cast from elevated areas that are
far-off. For those, the threshold may need to be raised to as much as
0.01.

The first step that will happen when this is launched is computing the
exposures. That one can be a bit slow, and can be done offline, using
the flag ``--compute-exposures-only`` in this tool, and then the
computed exposures can be passed to the command above via the
``--image-exposures-prefix`` option.

One should experiment with floating the albedo (option
``--float-albedo``) if noticeable albedo variations are seen in the
images. See :numref:`sfs_albedo` for a longer discussion.

To get more seamless results around small shadowed craters reduce the
value of ``--min-blend-size``. If you have many such
craters very close to each other, this may result in some erosion,
however.

When it comes to selecting the number of nodes to use, it is good to
notice how many tiles the ``parallel_sfs`` program produces (the tool
prints that), as a process will be launched for each tile. Since above
it is chosen to run 20 processes on each node, the number of nodes can
be the number of tiles over 20, or perhaps half or a quarter of that,
in which case it will take longer to run things. One should examine
how much memory these processes use and adjust this number
accordingly.

See :numref:`sfs_crater_bottoms` for a potential solution for SfS 
producing flat crater bottoms where there is no illumination to guide
the solver.

Inspection and further iterations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The obtained shape-from-shading terrain should be studied carefully to
see if it shows any systematic shift or rotation compared to the
initial LOLA gridded terrain. For that, the SfS terrain can be
overlayed as a georeferenced image on top of the initial terrain in
``stereo_gui``, and the SfS terrain can be toggled on and off.

If that is the case, another step of alignment can be used. This time
one can do features-based alignment rather than based on
point-to-point calculations. This works better on lower-resolution
versions of the inputs, when the high-frequency discrepancies do not
confuse the alignment, so, for example, at 1/4 or 1/8 resolution of
the DEMs, as created ``stereo_gui``::

    pc_align --initial-transform-from-hillshading rigid \
      ref_sub4.tif sfs_dem_sub4.tif -o align_sub4/run   \
      --num-iterations 0 --max-displacement -1

That alignment transform can then be applied to the full SfS DEM::

    pc_align --initial-transform align_sub4/run-transform.txt      \
      ref.tif sfs_dem.tif -o align/run --num-iterations 0          \
      --max-displacement -1 --save-transformed-source-points       \
      --max-num-reference-points 1000 --max-num-source-points 1000

(The number of points being used is not important since we will just
apply the alignment and transform the full DEM.)

The aligned SfS DEM can be regenerated from the obtained transformed
cloud as::

    point2dem --tr 1 --search-radius-factor 2 --t_srs projection_str \
      align/run-trans_source.tif

Here, the projection string should be the same one as in the reference 
LOLA DEM named ref.tif. It can be found by invoking::

    gdalinfo -proj4 ref.tif

and looking for the value of the ``PROJ.4`` field.

It is worth experimenting repeating this experiment at sub2 and sub8,
and examine visually the obtained hillshaded DEMs overlaid on top of
the reference DEM and see which agree with the reference the most
(even though the SfS DEM and the reference DEM can be quite different,
it is possible to notice subtle shifts upon careful inspection).

If this approach fails to remove the visually noticeable displacement
between the SfS and LOLA terrain, one can try to nudge the SfS terrain
manually, by using ``pc_align`` as::

    pc_align --initial-ned-translation                             \
      "north_shift east_shift down_shift"                          \
      ref.tif sfs_dem.tif -o align/run --num-iterations 0          \
      --max-displacement -1 --save-transformed-source-points       \
      --max-num-reference-points 1000 --max-num-source-points 1000

Here, value of ``down_shift`` should be 0, as we attempt a horizontal
shift. For the other ones one may try some values and observe their
effect in moving the SfS terrain to the desired place. The transform
obtained by using these numbers will be saved in
``align/run-transform.txt`` (while being converted from the local
North-East-Down coordinates to ECEF) and can be used below instead of
the transform obtained with invoking
``--initial-transform-from-hillshading``.

If a manual rotation nudge is necessary, use ``pc_align`` with
``--initial-rotation-angle``.

The transformed cloud then need to be regridded with ``point2dem``
as before.

It is very recommended to redo the whole process using the improved
alignment. First, the alignment transform must be applied to the
camera adjustments, by invoking bundle adjustment as earlier, with the
best cameras so far provided via ``--input-adjustments-prefix`` and
the latest ``pc_align`` transform passed to ``--initial-transform``
and the switch ``--apply-initial-transform-only``. Then, another pass of
bundle adjustment while doing registration to the ground should take
place as earlier, with ``--heights-from-dem`` and other related
options. Lastly mapprojection and SfS should be repeated. (Any bundle
adjustment operation can reuse the match files from previous attempts
if copying them over to the new output directory.)

Ideally, after all this, there should be no systematic offset
between the SfS terrain and the reference LOLA terrain.
 
Comparison with initial terrain and image mosaic
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``geodiff`` tool can be deployed to see how the SfS DEM compares
to the initial guess or to the raw ungridded LOLA measurements.
One can use the ``--absolute`` option for this tool and then invoke
``colormap`` to colorize the difference map. By and large, the SfS
DEM should not differ from the reference DEM by more than 1-2 meters.

To create a maximally-lit mosaic one can mosaic together all the mapprojected
images using the same camera adjustments that were used for SfS. That is
done as follows::

    dem_mosaic --max -o max_lit.tif image1.map.tif ... imageN.map.tif

Inspecting this image will show where misregistration occurred, as those
locations will show up blurry in this mosaic.

Handling issues
^^^^^^^^^^^^^^^

After an SfS solution was found, with the cameras well-adjusted to
each other and to the ground, and it is desired to add new camera
images (or perhaps fix some of the existing poorly aligned cameras),
one can create .adjust files for the new camera images (by perhaps
using the identity adjustment, see :numref:`adjust_files` for the
format of such files), run bundle adjustment again with the
supplemented set of camera adjustments as initial guess using
``--input-adjustments-prefix``, and one may keep fixed the cameras for
which the adjustment is already good using the option
``--fixed-camera-indices``.

If in some low-light locations the SfS DEM still has seams, one may
consider invoking ``sfs`` with ``--robust-threshold 0.004``, removing
some of the offending images, or with a larger value for
``--shadow-threshold`` (such as 0.007 or 0.015) for those images, or a
larger value for ``--blending-dist``. A per-image shadow threshold
which overrides the globally set value can be specified via
``--custom-shadow-threshold-list``. Sometimes this improves the
solution in some locations while introducing artifacts in other.

If the SfS DEM has localized defects, those can be fixed in a small
region and then blended in. For example, a clip around the defect,
perhaps of dimensions 150-200 pixels, can be cut from the input
DEM. If that clip has noise which affects the final SfS result, it can
be blurred with ``dem_mosaic``, using for example, ``--dem-blur-sigma
5``. Then one can try to run SfS on just this clip, and if needed vary
some of the SfS parameters or exclude some images. If happy enough
with the result, this SfS clip can be blended back to the larger SfS
DEM with ``dem_mosaic`` with the ``--priority-blending-length``
option, whose value can be set, for example, to 50, to blend over this
many pixels inward from the boundary of the clip to be inserted.

Blending the SfS result with the initial terrain
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After computing a satisfactory SfS DEM, it can be processed to replace
the values in the permanently shadowed areas with values from the
original LOLA DEM, with a transition region. That can be done as::

    sfs_blend --lola-dem lola_dem.tif --sfs-dem sfs_dem.tif      \
      --max-lit-image-mosaic max_lit.tif --image-threshold 0.005 \
      --lit-blend-length 25 --shadow-blend-length 5              \
      --min-blend-size 50 --weight-blur-sigma 5                  \
      --output-dem sfs_blend.tif --output-weight sfs_weight.tif

Here, the inputs are the LOLA and SfS DEMs, the maximally lit mosaic
provided as before, the shadow threshold (the same value as in
invoking SfS should be used). These are expected to have
precisely the same extent, projection, and resolution.

The outputs are the blended DEM as described earlier, and the weight
which tells how much the SfS DEM contributed to the blended DEM. That
weight equals to 1 where only the SfS DEM was used, is between 0 and 1
in the transition region between the lit and shadowed areas, which is
determined by the values of the ``--lit-blend-length`` and
``--shadow-blend-length`` parameters (it grows somewhat depending on
the value of ``--weight-blur-sigma``), and is 0 where only the LOLA
values contribute to the solution. The weight function is the
truncated signed Euclidean distance to the lit boundary, scaled to have
values between 0 and 1, then blurred with a Gaussian kernel with the
above-mentioned sigma. No blending happens for shadowed regions of
dimensions less than `--min-blend-size`, where the SfS DEM is
kept. See :numref:`sfs_blend` for more details.

(Note that if one tries to blend an SfS terrain obtained after
``pc_align``, that won't have the same extent as the LOLA terrain,
which will make this command fail. It is suggested that the input LOLA
terrain be prepared with ``gdalwarp -te <corners>`` as described
earlier, and then the SfS terrain be regenerated starting with this
terrain, with any desired transform applied to the cameras before
``parallel_sfs`` is rerun, and then the extent of the LOLA and SfS
terrains will agree. Or, though this is not recommended, the SfS
terrain which exists so far and the LOLA terrain can both be
interpolated using the same ``gdalwarp -te <corners>`` command, or with 
``dem_mosaic --tap`` as mentioned above.) Any invocation of ``gdalwarp``
should be used with bicubic or other smooth interpolation.

Creation of mask of SfS pixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The SfS DEM used the information from images where the ground
was lit, so above the shadow threshold. In the shadowed areas just 
the smoothness constraint and initial DEM constraint were used.
To create the mask of such lit pixels, with value of 1 where
lit and 0 where unlit, use the maximally-lit mosaic found earlier,
and run::

    thresh=0.005
    image_calc -c "sign(max(var_0, $thresh) - $thresh)" \
      max_lit.tif -o sfs_mask.tif

Here, the shadow threshold used during SfS should be used, separating
lit and unlit pixels.

SfS height uncertainty map
^^^^^^^^^^^^^^^^^^^^^^^^^^

The error in the SfS DEM (before or after the blending with LOLA) 
can be estimated as::

   parallel_sfs --estimate-height-errors -i sfs_dem.tif \
    -o sfs_error/run <other options as above>

See the :numref:`sfs` describing how the estimation is
implemented. 

.. _sfsinsights:

Insights for getting the most of SfS
------------------------------------

Here are a few suggestions we have found helpful when running ``sfs``:

- First determine the appropriate smoothing weight :math:`\mu` by
  running a small clip, and using just one image. A value between 0.06
  and 0.12 seems to work all the time with LRO NAC, even when the
  images are subsampled. The other weight, :math:`\lambda,` 
  that is, the value of ``--initial-dem-constraint-weight``, can be
  set to something small, like :math:`0.0001.` This can be increased to
  :math:`0.001` if noticing that the output DEM strays too far.

- Bundle-adjustment for multiple images and alignment to ground is
  crucial, to eliminate camera errors which will result in ``sfs``
  converging to a local minimum. This is described in
  :numref:`sfs-lola-comparison`.

- More images with more diverse illumination conditions result in more 
  accurate terrain. Ideally there should be at least 3 images, with the 
  shadows being, respectively, on the left, right, and then perhaps 
  missing or small. Images with intermediate illumination conditions may 
  be needed for bundle adjustment to work.

- Floating the albedo (option ``--float-albedo``) can introduce
  instability and divergence, it should be avoided unless obvious
  albedo variation is seen in the images. See :numref:`sfs_albedo` for
  a longer discussion.

- Floating the cameras in SfS should be avoided, as bundle adjustment
  does a better job. Floating the exposures was shown to produce
  marginal results.

- Floating the DEM at the boundary (option ``--float-dem-at-boundary``)
  is also suggested to be avoided.

- If an input DEM is large, see the :numref:`sfs-lola-dem` for a detailed
  recipe using ``parallel_sfs``.

- The ``mapproject`` program can be used to map-project each image onto
  the resulting SfS DEM (with the camera adjustments solved using
  ``bundle_adjust``). These orthoimages can be mosaicked using
  ``dem_mosaic``. If the ``--max`` option is used with this tool, it
  create a mosaic with the most illuminated pixels from this image. If
  the camera adjustments were solved accurately, this mosaic should
  have little or no blur or misregistration (ghosting).

- For challenging datasets it is suggested to first work at 1/4th of
  the full resolution (the resolution of an ISIS cube can be changed
  using the ``reduce`` command, and the DEM can be made coarser with
  ``gdalwarp`` or by converting it to a cloud with ``pc_align`` with
  zero iterations and then regenerated with ``point2dem``). This should
  make the whole process perhaps an order of magnitude faster. Any
  obtained camera adjustment files are still usable at the full
  resolution (after an appropriate rename), but it is suggested that
  these adjustments be reoptimized using the full resolution cameras,
  hence these should be initial guesses for ``bundle_adjust``'s
  ``--input-adjustments-prefix`` option, and also using the
  ``--heights-from-dem`` option.

 .. |times| unicode:: U+00D7 .. MULTIPLICATION SIGN
