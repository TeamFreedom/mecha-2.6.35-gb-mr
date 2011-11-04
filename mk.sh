#!/bin/sh
T=$PWD
PRODUCT=Zoom-Kernel
PRODUCT_CONFIG_FILE=mecha-lte_defconfig
RAMDISK_FILES=ramdisk
MKRAMDISK=$T/tools/update-zip-tools/mkbootfs
MKBOOTIMG=$T/tools/update-zip-tools/mkbootimg
VERSION=1.0.2
DATE=$(date +%Y-%m-%d)

echo " === Cleaning up ==="
rm -rf $T/out
mkdir -p $T/out
make ARCH=arm clean
echo " === Setting up the build configuration ==="
make ARCH=arm defconfig $PRODUCT_CONFIG_FILE
echo " === Starting the build ==="
make ARCH=arm -j50
if [ -f "$T/arch/arm/boot/zImage" ];
then
    echo " === zImage found, continuing to packaging ==="
else
    echo " === zImage not found, your build has failed... ==="
fi
mkdir -p $T/out/$PRODUCT
mkdir -p $T/out/$PRODUCT/kernel
mkdir -p $T/out/$PRODUCT/system/lib/modules
mkdir -p $T/out/$PRODUCT/META-INF/com/google/android
mv $T/arch/arm/boot/zImage $T/out/$PRODUCT/kernel/zImage
mv $T/drivers/net/wireless/bcm4329/bcm4329.ko $T/out/$PRODUCT/system/lib/modules/bcm4329.ko
cp -R $T/tools/update-zip-tools/update-binary $T/out/$PRODUCT/META-INF/com/google/android/update-binary
cp -R $T/tools/update-zip-tools/updater-script $T/out/$PRODUCT/META-INF/com/google/android/updater-script
cp -R $T/tools/update-zip-tools/flash_image $T/out/$PRODUCT/flash_image
$MKRAMDISK $T/tools/$RAMDISK_FILES | gzip > $T/out/$PRODUCT/boot.img-ramdisk.cpio.gz
$MKBOOTIMG --kernel $T/out/$PRODUCT/kernel/zImage --ramdisk $T/out/$PRODUCT/boot.img-ramdisk.cpio.gz --cmdline no_console_suspend=1 --base 0x05200000 --pagesize 4096 -o $T/out/$PRODUCT/boot.img
cd $T/out/$PRODUCT
zip -r $PRODUCT-$VERSION-$DATE.zip META-INF/ system/ boot.img flash_image
OUTPUT_ZIP=$PRODUCT-$VERSION-$DATE
cd $T
java -jar $T/tools/update-zip-tools/signapk.jar $T/tools/update-zip-tools/testkey.x509.pem $T/tools/update-zip-tools/testkey.pk8 $T/out/$PRODUCT/$OUTPUT_ZIP.zip $T/out/$PRODUCT/$OUTPUT_ZIP-signed.zip
rm $T/out/$PRODUCT/$OUTPUT_ZIP.zip
echo " Build is now complete. Package and raw files are now in /out"
