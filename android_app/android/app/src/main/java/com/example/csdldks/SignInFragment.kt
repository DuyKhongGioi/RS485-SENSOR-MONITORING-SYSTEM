package com.example.csdldks

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import com.google.android.material.button.MaterialButton
import com.google.android.material.textfield.TextInputEditText
import com.example.csdldks.databinding.FragmentSignInBinding
import com.google.firebase.auth.FirebaseAuth

class SignInFragment : Fragment() {
    private var _binding: FragmentSignInBinding? = null
    private val binding get() = _binding!!
    private lateinit var auth: FirebaseAuth

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        auth = FirebaseAuth.getInstance()
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentSignInBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.btnSignIn.setOnClickListener {
            val email = binding.edtEmail.text?.toString()?.trim()
            val password = binding.edtPassword.text?.toString()?.trim()

            if (email.isNullOrEmpty()) {
                binding.edtEmail.error = "Vui lòng nhập email"
                return@setOnClickListener
            }
            if (password.isNullOrEmpty()) {
                binding.edtPassword.error = "Vui lòng nhập mật khẩu"
                return@setOnClickListener
            }

            auth.signInWithEmailAndPassword(email, password)
                .addOnCompleteListener { task ->
                    if (task.isSuccessful) {
                        Toast.makeText(context, "Đăng nhập thành công!", Toast.LENGTH_SHORT).show()
                        findNavController().navigate(R.id.action_signIn_to_home)
                    } else {
                        Toast.makeText(context, "Đăng nhập thất bại: ${task.exception?.message}", Toast.LENGTH_SHORT).show()
                    }
                }
        }

        binding.tvSignUp.setOnClickListener {
            findNavController().navigate(R.id.action_signIn_to_signUp)
        }

        binding.tvForgotPassword.setOnClickListener {
            val email = binding.edtEmail.text?.toString()?.trim()

            if (email.isNullOrEmpty()) {
                binding.edtEmail.error = "Vui lòng nhập email để khôi phục mật khẩu"
                return@setOnClickListener
            }

            auth.sendPasswordResetEmail(email)
                .addOnCompleteListener { task ->
                    if (task.isSuccessful) {
                        Toast.makeText(context, "Email khôi phục mật khẩu đã được gửi!", Toast.LENGTH_SHORT).show()
                    } else {
                        Toast.makeText(context, "Lỗi: ${task.exception?.message}", Toast.LENGTH_SHORT).show()
                    }
                }
        }

    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}