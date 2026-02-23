package com.example.csdldks

import android.graphics.Color
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import com.example.csdldks.databinding.FragmentHomeBinding
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.github.mikephil.charting.formatter.IndexAxisValueFormatter
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.*
import kotlinx.coroutines.launch

class HomeFragment : Fragment() {
    private var _binding: FragmentHomeBinding? = null
    private val binding get() = _binding!!
    private lateinit var database: DatabaseReference
    private lateinit var auth: FirebaseAuth
    private var temperatureChart: LineChart? = null
    private var humidityChart: LineChart? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        auth = FirebaseAuth.getInstance()
        database = FirebaseDatabase.getInstance(
            "https://temperaturehumidity-4f9f0-default-rtdb.asia-southeast1.firebasedatabase.app/"
        ).reference.child("sensors")
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentHomeBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val user = auth.currentUser
        binding.welcomeText.text = "Chào mừng, ${user?.displayName ?: "Người dùng"}!"

        temperatureChart = binding.temperatureChart
        humidityChart = binding.humidityChart

        if (temperatureChart == null || humidityChart == null) {
            Toast.makeText(context, "Biểu đồ không được tìm thấy trong layout", Toast.LENGTH_SHORT).show()
            Log.e("HomeFragment", "temperatureChart hoặc humidityChart là null")
            return
        }

        setupCharts()
        fetchSensorData()
    }

    private fun setupCharts() {
        temperatureChart?.apply {
            description.isEnabled = false
            setDrawGridBackground(false)
            axisLeft.apply {
                setDrawGridLines(true)
                textSize = 12f
                axisMinimum = -10f
                axisMaximum = 50f
            }
            axisRight.isEnabled = false
            xAxis.apply {
                setDrawGridLines(false)
                textSize = 12f
                granularity = 1f
                valueFormatter = IndexAxisValueFormatter() // Hiển thị giá trị index nguyên
            }
            legend.isEnabled = true
            animateX(1000)
        }

        humidityChart?.apply {
            description.isEnabled = false
            setDrawGridBackground(false)
            axisLeft.apply {
                setDrawGridLines(true)
                textSize = 12f
                axisMinimum = 0f
                axisMaximum = 100f
            }
            axisRight.isEnabled = false
            xAxis.apply {
                setDrawGridLines(false)
                textSize = 12f
                granularity = 1f
                valueFormatter = IndexAxisValueFormatter() // Hiển thị giá trị index nguyên
            }
            legend.isEnabled = true
            animateX(1000)
        }
    }

    private fun fetchSensorData() {
        val sensors = listOf("sensor1", "sensor2", "sensor3")
        val tempDataSets = mutableMapOf<String, LineDataSet>()
        val humDataSets = mutableMapOf<String, LineDataSet>()
        val colors = listOf(Color.RED, Color.BLUE, Color.GREEN)

        sensors.forEachIndexed { index, sensorId ->
            val tempDataSet = LineDataSet(null, "Cảm biến $sensorId - Nhiệt độ").apply {
                color = colors[index]
                setDrawCircles(true)
                circleRadius = 3f
                lineWidth = 2f
            }
            val humDataSet = LineDataSet(null, "Cảm biến $sensorId - Độ ẩm").apply {
                color = colors[index]
                setDrawCircles(true)
                circleRadius = 3f
                lineWidth = 2f
            }
            tempDataSets[sensorId] = tempDataSet
            humDataSets[sensorId] = humDataSet
        }

        sensors.forEach { sensorId ->
            database.child(sensorId).child("readings")
                .limitToLast(10)
                .addValueEventListener(object : ValueEventListener {
                    override fun onDataChange(snapshot: DataSnapshot) {
                        val tempEntries = mutableListOf<Entry>()
                        val humEntries = mutableListOf<Entry>()

                        // Sử dụng index từ 0 đến số lượng readings - 1
                        snapshot.children.forEachIndexed { index, data ->
                            val temperature = data.child("temperature").getValue(Float::class.java)
                            val humidity = data.child("humidity").getValue(Float::class.java)

                            if (temperature != null && humidity != null &&
                                !temperature.isNaN() && !humidity.isNaN()) {
                                tempEntries.add(Entry(index.toFloat(), temperature))
                                humEntries.add(Entry(index.toFloat(), humidity))
                            } else {
                                Log.w("HomeFragment", "Sensor $sensorId có dữ liệu không hợp lệ: $temperature / $humidity")
                            }
                        }

                        Log.d("FirebaseSnapshot", "Sensor $sensorId - Tổng readings: ${snapshot.childrenCount}, temp: ${tempEntries.size}, hum: ${humEntries.size}")

                        // Cập nhật dữ liệu trên main thread
                        activity?.runOnUiThread {
                            tempDataSets[sensorId]?.values = tempEntries
                            humDataSets[sensorId]?.values = humEntries

                            if (tempEntries.isNotEmpty() || humEntries.isNotEmpty()) {
                                updateCharts(tempDataSets, humDataSets)
                            }
                        }
                    }

                    override fun onCancelled(error: DatabaseError) {
                        Toast.makeText(context, "Lỗi tải dữ liệu: ${error.message}", Toast.LENGTH_SHORT).show()
                        Log.e("HomeFragment", "Firebase error: ${error.message}")
                    }
                })
        }
    }

    private fun updateCharts(
        tempDataSets: Map<String, LineDataSet>,
        humDataSets: Map<String, LineDataSet>
    ) {
        val tempLineData = LineData()
        tempDataSets.values.forEach { dataSet ->
            if (dataSet.entryCount > 0) {
                tempLineData.addDataSet(dataSet)
            }
        }
        temperatureChart?.apply {
            data = tempLineData
            notifyDataSetChanged()
            invalidate()
        }

        val humLineData = LineData()
        humDataSets.values.forEach { dataSet ->
            if (dataSet.entryCount > 0) {
                humLineData.addDataSet(dataSet)
            }
        }
        humidityChart?.apply {
            data = humLineData
            notifyDataSetChanged()
            invalidate()
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
        temperatureChart = null
        humidityChart = null
    }
}