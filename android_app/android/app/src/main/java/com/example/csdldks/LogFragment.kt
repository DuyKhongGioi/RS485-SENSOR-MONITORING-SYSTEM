package com.example.csdldks

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import com.example.csdldks.databinding.FragmentLogBinding
import com.google.firebase.database.*
import kotlinx.coroutines.launch

class LogFragment : Fragment() {
    private var _binding: FragmentLogBinding? = null
    private val binding get() = _binding!!
    private lateinit var database: DatabaseReference
    private val logEntries = mutableListOf<Pair<String, String>>()
    private var displayedLogCount = 50 // Initial limit
    private val batchSize = 50 // Number of logs to load each time
    private var valueEventListener: ValueEventListener? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        database = FirebaseDatabase.getInstance(
            "https://temperaturehumidity-4f9f0-default-rtdb.asia-southeast1.firebasedatabase.app/"
        ).reference.child("logs")
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentLogBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.logText.text = "Activity Log\n"

        binding.viewMoreButton?.setOnClickListener {
            displayedLogCount += batchSize
            updateLogDisplay()
        }

        valueEventListener = object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                logEntries.clear()

                for (logEntry in snapshot.children) {
                    val event = logEntry.child("event").getValue(String::class.java) ?: "Unknown Event"
                    val timestamp = logEntry.child("timestamp").getValue(String::class.java) ?: "No Timestamp"
                    logEntries.add(Pair(event, timestamp))
                }

                // Sắp xếp an toàn: bỏ qua nếu timestamp không hợp lệ
                try {
                    logEntries.sortByDescending { it.second }
                } catch (e: Exception) {
                    // Nếu sắp xếp thất bại, giữ nguyên thứ tự
                }

                // Cập nhật UI trên main thread một cách an toàn
                viewLifecycleOwner.lifecycleScope.launch {
                    updateLogDisplay()
                }
            }

            override fun onCancelled(error: DatabaseError) {
                viewLifecycleOwner.lifecycleScope.launch {
                    if (_binding != null) {
                        binding.logText.text = "Failed to load logs: ${error.message}"
                    }
                }
            }
        }.also { listener ->
            database.orderByChild("timestamp").addValueEventListener(listener)
        }
    }

    private fun updateLogDisplay() {
        // Kiểm tra trạng thái fragment và binding
        if (!isAdded || _binding == null) return

        val logBuilder = StringBuilder()
        logBuilder.append("Activity Log\n\n")

        val logsToShow = logEntries.take(displayedLogCount)
        for ((event, timestamp) in logsToShow) {
            logBuilder.append("Event: $event\nTimestamp: $timestamp\n\n")
        }

        binding.logText.text = logBuilder.toString()
        binding.viewMoreButton?.visibility = if (displayedLogCount < logEntries.size) View.VISIBLE else View.GONE
    }

    override fun onDestroyView() {
        super.onDestroyView()
        valueEventListener?.let { database.removeEventListener(it) }
        valueEventListener = null
        _binding = null
    }
}