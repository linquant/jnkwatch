package fr.ponera.jnkwatch.weather

import android.annotation.SuppressLint
import android.content.Context
import android.location.Geocoder
import android.location.Location
import android.location.LocationManager
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.util.Locale

data class Weather(val tempC: Int, val cond: Int, val location: String)

/**
 * Fetches current weather for the phone's location from Open-Meteo (free, no key)
 * and maps it to the watch's condition enum. Uses only built-in APIs
 * (LocationManager, HttpURLConnection, org.json, Geocoder) — no extra deps.
 */
object WeatherRepo {

    @SuppressLint("MissingPermission") // permission checked by the UI before use
    private fun lastLocation(context: Context): Location? {
        val lm = context.getSystemService(Context.LOCATION_SERVICE) as? LocationManager ?: return null
        val providers = listOf(
            LocationManager.NETWORK_PROVIDER,
            LocationManager.GPS_PROVIDER,
            LocationManager.PASSIVE_PROVIDER,
        )
        var best: Location? = null
        for (p in providers) {
            val l = try { lm.getLastKnownLocation(p) } catch (_: Exception) { null }
            if (l != null && (best == null || l.time > best!!.time)) best = l
        }
        return best
    }

    /** Open-Meteo WMO weather_code -> watch condition (see docs/ble-protocol.md). */
    private fun wmoToCond(code: Int): Int = when (code) {
        0 -> 0
        1, 2 -> 1
        3 -> 2
        45, 48 -> 3
        in 51..67, in 80..82 -> 4
        in 71..77, 85, 86 -> 5
        95, 96, 99 -> 6
        else -> 2
    }

    private fun cityName(context: Context, lat: Double, lon: Double): String = try {
        @Suppress("DEPRECATION")
        val addrs = Geocoder(context, Locale.getDefault()).getFromLocation(lat, lon, 1)
        addrs?.firstOrNull()?.let { it.locality ?: it.subAdminArea ?: it.adminArea }.orEmpty()
    } catch (_: Exception) {
        ""
    }

    private fun httpGet(url: String): String? = try {
        val c = (URL(url).openConnection() as HttpURLConnection).apply {
            connectTimeout = 8000; readTimeout = 8000; requestMethod = "GET"
        }
        c.inputStream.bufferedReader().use { it.readText() }
    } catch (_: Exception) {
        null
    }

    /** Fetch current weather at the phone's location, or null on any failure. */
    suspend fun fetch(context: Context): Weather? = withContext(Dispatchers.IO) {
        val loc = lastLocation(context) ?: return@withContext null
        val lat = loc.latitude
        val lon = loc.longitude
        val url = "https://api.open-meteo.com/v1/forecast?latitude=$lat&longitude=$lon" +
            "&current=temperature_2m,weather_code&timezone=auto"
        val body = httpGet(url) ?: return@withContext null
        val cur = JSONObject(body).optJSONObject("current") ?: return@withContext null
        val temp = Math.round(cur.optDouble("temperature_2m", 0.0)).toInt()
        val cond = wmoToCond(cur.optInt("weather_code", 3))
        Weather(temp, cond, cityName(context, lat, lon))
    }
}
