package fr.ponera.jnkwatch.firmware

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Test

class FirmwareRepoTest {

    private fun body(
        sha: String = "ab".repeat(32),
        major: Int = 1,
        minor: Int = 4,
    ) = """{"major":$major,"minor":$minor,"size":1395072,"sha256":"$sha",
            "url":"https://example.com/jnkwatch.bin"}"""

    private val manifest = FirmwareRepo.Manifest(1, 4, 4, ByteArray(32), "https://example.com/jnkwatch.bin")

    @Test
    fun parsesAValidManifest() {
        val m = FirmwareRepo.parseManifest(body())
        assertEquals(1, m!!.major)
        assertEquals(4, m.minor)
        assertEquals(1395072, m.size)
        assertEquals(32, m.sha256.size)
        assertEquals(0xAB.toByte(), m.sha256[0])
        assertEquals("https://example.com/jnkwatch.bin", m.url)
    }

    @Test
    fun rejectsAShortSha() {
        assertNull(FirmwareRepo.parseManifest(body(sha = "ab")))
    }

    @Test
    fun rejectsNonHexSha() {
        assertNull(FirmwareRepo.parseManifest(body(sha = "zz".repeat(32))))
    }

    @Test
    fun rejectsMalformedJsonOrMissingField() {
        assertNull(FirmwareRepo.parseManifest("not json"))
        assertNull(FirmwareRepo.parseManifest("""{"major":1,"minor":4}"""))
    }

    @Test
    fun stableOffersOnlyNewerVersions() {
        val stable = FirmwareRepo.Channel.STABLE
        assertSame(manifest, FirmwareRepo.offerFor(stable, manifest, 1 to 3))
        assertNull(FirmwareRepo.offerFor(stable, manifest, 1 to 4))
        assertNull(FirmwareRepo.offerFor(stable, manifest, 2 to 0))
    }

    @Test
    fun devAlwaysOffersItsBuild() {
        val dev = FirmwareRepo.Channel.DEV
        assertSame(manifest, FirmwareRepo.offerFor(dev, manifest, 1 to 4))
        assertSame(manifest, FirmwareRepo.offerFor(dev, manifest, 2 to 0))
    }

    @Test
    fun nothingToOfferWithoutAManifestOrAWatchVersion() {
        assertNull(FirmwareRepo.offerFor(FirmwareRepo.Channel.DEV, null, 1 to 4))
        assertNull(FirmwareRepo.offerFor(FirmwareRepo.Channel.DEV, manifest, null))
    }
}
