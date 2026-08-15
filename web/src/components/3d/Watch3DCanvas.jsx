import React, { useEffect, useRef, useState } from 'react';
import * as THREE from 'three';
import { Heart, Activity, ShieldAlert, Cpu, RefreshCw, AlertTriangle } from 'lucide-react';

export default function Watch3DCanvas({ onTriggerSOS, simulatedBpm = 76 }) {
  const mountRef = useRef(null);
  const [activeScreenTab, setActiveScreenTab] = useState('HOME');
  const [isRotating, setIsRotating] = useState(true);

  useEffect(() => {
    const currentMount = mountRef.current;
    if (!currentMount) return;

    const width = currentMount.clientWidth;
    const height = currentMount.clientHeight;

    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 1000);
    camera.position.set(0, 0, 7.2);

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(width, height);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.shadowMap.enabled = true;
    currentMount.appendChild(renderer.domElement);

    const ambientLight = new THREE.AmbientLight(0xffffff, 0.9);
    scene.add(ambientLight);

    const dirLight1 = new THREE.DirectionalLight(0x00f2fe, 1.8);
    dirLight1.position.set(5, 5, 5);
    scene.add(dirLight1);

    const dirLight2 = new THREE.DirectionalLight(0xff3366, 1.2);
    dirLight2.position.set(-5, -5, 2);
    scene.add(dirLight2);

    const watchGroup = new THREE.Group();

    // Watch Case Body (Matte Black Finish)
    const caseGeo = new THREE.CylinderGeometry(2.1, 2.1, 0.45, 64);
    const caseMat = new THREE.MeshStandardMaterial({
      color: 0x0f172a,
      metalness: 0.8,
      roughness: 0.3,
    });
    const watchCase = new THREE.Mesh(caseGeo, caseMat);
    watchCase.rotation.x = Math.PI / 2;
    watchGroup.add(watchCase);

    // Bezel Ring
    const bezelGeo = new THREE.TorusGeometry(2.12, 0.07, 16, 64);
    const bezelMat = new THREE.MeshStandardMaterial({
      color: 0x00f2fe,
      metalness: 0.9,
      roughness: 0.15,
      emissive: 0x00f2fe,
      emissiveIntensity: 0.2
    });
    const bezel = new THREE.Mesh(bezelGeo, bezelMat);
    watchGroup.add(bezel);

    // Side Button
    const buttonGeo = new THREE.CylinderGeometry(0.18, 0.18, 0.35, 32);
    const buttonMat = new THREE.MeshStandardMaterial({
      color: 0xff3366,
      metalness: 0.6,
      roughness: 0.3,
      emissive: 0xff3366,
      emissiveIntensity: 0.4
    });
    const button = new THREE.Mesh(buttonGeo, buttonMat);
    button.position.set(2.25, 0, 0);
    button.rotation.z = Math.PI / 2;
    watchGroup.add(button);

    // Watch Straps
    const strapGeo = new THREE.BoxGeometry(1.4, 2.2, 0.18);
    const strapMat = new THREE.MeshStandardMaterial({ color: 0x1e293b, roughness: 0.7 });
    const topStrap = new THREE.Mesh(strapGeo, strapMat);
    topStrap.position.set(0, 2.8, -0.05);
    watchGroup.add(topStrap);

    const bottomStrap = new THREE.Mesh(strapGeo, strapMat);
    bottomStrap.position.set(0, -2.8, -0.05);
    watchGroup.add(bottomStrap);

    // Screen Disc
    const screenGeo = new THREE.CircleGeometry(1.98, 64);
    const canvasTexture = createScreenTexture(simulatedBpm, activeScreenTab);
    const screenMat = new THREE.MeshBasicMaterial({ map: canvasTexture });
    const screen = new THREE.Mesh(screenGeo, screenMat);
    screen.position.z = 0.23;
    watchGroup.add(screen);

    // Particles
    const particlesGeo = new THREE.BufferGeometry();
    const particleCount = 50;
    const posArray = new Float32Array(particleCount * 3);

    for (let i = 0; i < particleCount * 3; i += 3) {
      const radius = 2.7 + Math.random() * 1.2;
      const theta = Math.random() * Math.PI * 2;
      const phi = (Math.random() - 0.5) * Math.PI;
      posArray[i] = radius * Math.cos(theta) * Math.cos(phi);
      posArray[i + 1] = radius * Math.sin(theta) * Math.cos(phi);
      posArray[i + 2] = radius * Math.sin(phi);
    }
    particlesGeo.setAttribute('position', new THREE.BufferAttribute(posArray, 3));

    const particleMat = new THREE.PointsMaterial({
      size: 0.04,
      color: 0x00f2fe,
      transparent: true,
      opacity: 0.5,
    });
    const particleSystem = new THREE.Points(particlesGeo, particleMat);
    scene.add(particleSystem);

    scene.add(watchGroup);

    watchGroup.rotation.x = 0.25;
    watchGroup.rotation.y = -0.35;

    let mouseX = 0;
    let mouseY = 0;

    const handleMouseMove = (event) => {
      const rect = currentMount.getBoundingClientRect();
      const x = (event.clientX - rect.left) / width;
      const y = (event.clientY - rect.top) / height;
      mouseX = (x - 0.5) * 2;
      mouseY = (y - 0.5) * 2;
    };

    currentMount.addEventListener('mousemove', handleMouseMove);

    let animationFrameId;
    let clock = new THREE.Clock();

    const animate = () => {
      animationFrameId = requestAnimationFrame(animate);
      const elapsedTime = clock.getElapsedTime();

      particleSystem.rotation.y = elapsedTime * 0.1;

      if (isRotating) {
        watchGroup.rotation.y += 0.004;
      } else {
        const targetY = mouseX * 0.7;
        const targetX = mouseY * 0.7 + 0.2;
        watchGroup.rotation.y += (targetY - watchGroup.rotation.y) * 0.05;
        watchGroup.rotation.x += (targetX - watchGroup.rotation.x) * 0.05;
      }

      watchGroup.position.y = Math.sin(elapsedTime * 1.2) * 0.08;

      renderer.render(scene, camera);
    };

    animate();

    const handleResize = () => {
      if (!currentMount) return;
      const newW = currentMount.clientWidth;
      const newH = currentMount.clientHeight;
      camera.aspect = newW / newH;
      camera.updateProjectionMatrix();
      renderer.setSize(newW, newH);
    };

    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      currentMount.removeEventListener('mousemove', handleMouseMove);
      cancelAnimationFrame(animationFrameId);
      if (currentMount.contains(renderer.domElement)) {
        currentMount.removeChild(renderer.domElement);
      }
      renderer.dispose();
    };
  }, [simulatedBpm, activeScreenTab, isRotating]);

  // Screen texture rendering for "ENROLL WATCH"
  const createScreenTexture = (bpm, tab) => {
    const canvas = document.createElement('canvas');
    canvas.width = 512;
    canvas.height = 512;
    const ctx = canvas.getContext('2d');

    const grad = ctx.createRadialGradient(256, 256, 10, 256, 256, 256);
    grad.addColorStop(0, '#0f172a');
    grad.addColorStop(1, '#020617');
    ctx.fillStyle = grad;
    ctx.beginPath();
    ctx.arc(256, 256, 256, 0, Math.PI * 2);
    ctx.fill();

    ctx.strokeStyle = '#00f2fe';
    ctx.lineWidth = 4;
    ctx.beginPath();
    ctx.arc(256, 256, 245, 0, Math.PI * 2);
    ctx.stroke();

    // 1. ENROLL WATCH Title
    ctx.fillStyle = '#00f2fe';
    ctx.font = 'bold 24px Inter, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('ĐỒNG HỒ ENROLL', 256, 75);

    // Subtitle
    ctx.fillStyle = '#94a3b8';
    ctx.font = 'bold 15px Inter, sans-serif';
    ctx.fillText('TinyML AI INT8 Bio-Monitor', 256, 102);

    if (tab === 'HOME') {
      ctx.fillStyle = '#ffffff';
      ctx.font = 'bold 56px Inter, monospace';
      ctx.fillText('14:25', 256, 175);

      ctx.fillStyle = '#ff3366';
      ctx.font = 'bold 44px Inter, sans-serif';
      ctx.fillText(`♥ ${bpm} BPM`, 256, 250);

      ctx.fillStyle = '#00e676';
      ctx.font = 'bold 26px Inter, sans-serif';
      ctx.fillText('98% SpO2', 256, 320);

      ctx.fillStyle = '#cbd5e1';
      ctx.font = '14px Inter, sans-serif';
      ctx.fillText('5-Stage DSP • Normal', 256, 355);

      ctx.fillStyle = '#ffd700';
      ctx.font = 'bold 15px Inter, sans-serif';
      ctx.fillText('● AI FALL MONITOR: READY', 256, 435);

    } else if (tab === 'HR') {
      ctx.fillStyle = '#ff3366';
      ctx.font = 'bold 26px Inter, sans-serif';
      ctx.fillText('SÓNG NHỊP TIM ECG', 256, 155);

      ctx.fillStyle = '#ffffff';
      ctx.font = 'bold 68px Inter, monospace';
      ctx.fillText(`${bpm}`, 256, 245);

      ctx.fillStyle = '#00f2fe';
      ctx.font = 'bold 18px Inter, sans-serif';
      ctx.fillText('BPM • NHỊP ĐỀU CÂN BẰNG', 256, 290);

      ctx.strokeStyle = '#ff3366';
      ctx.lineWidth = 4;
      ctx.beginPath();
      ctx.moveTo(110, 370);
      ctx.lineTo(160, 370);
      ctx.lineTo(175, 345);
      ctx.lineTo(190, 400);
      ctx.lineTo(210, 310);
      ctx.lineTo(230, 410);
      ctx.lineTo(245, 370);
      ctx.lineTo(402, 370);
      ctx.stroke();

    } else if (tab === 'FALL') {
      ctx.fillStyle = '#ff3366';
      ctx.font = 'bold 24px Inter, sans-serif';
      ctx.fillText('⚠️ DỰ ĐOÁN TÉ NGÃ AI', 256, 155);

      ctx.fillStyle = '#ffffff';
      ctx.font = 'bold 22px Inter, sans-serif';
      ctx.fillText('Gia tốc IMU 3D: 1.02g', 256, 215);

      ctx.fillStyle = '#00f2fe';
      ctx.font = 'bold 18px Inter, sans-serif';
      ctx.fillText('Thỏa mãn 4 Giai đoạn', 256, 260);

      ctx.fillStyle = '#ffd700';
      ctx.font = 'bold 22px Inter, sans-serif';
      ctx.fillText('Độ chính xác AI: 99.4%', 256, 330);

      ctx.fillStyle = '#00e676';
      ctx.font = 'bold 16px Inter, sans-serif';
      ctx.fillText('Telegram SOS 15s Active', 256, 420);
    }

    const texture = new THREE.CanvasTexture(canvas);
    texture.needsUpdate = true;
    return texture;
  };

  return (
    <div className="relative w-full h-[470px] sm:h-[500px] glass-panel rounded-3xl overflow-hidden flex flex-col items-center justify-center p-3 sm:p-4 border border-slate-800 shadow-2xl">
      
      {/* Top Floating Controls Bar */}
      <div className="absolute top-4 left-4 right-4 flex items-center justify-between z-10">
        <button
          onClick={onTriggerSOS}
          className="px-3.5 py-1.5 bg-rose-600/90 hover:bg-rose-500 text-white font-bold text-xs rounded-xl shadow-lg shadow-rose-600/30 flex items-center space-x-1.5 transition-all transform hover:scale-105"
        >
          <AlertTriangle className="w-3.5 h-3.5 animate-bounce" />
          <span>Mô Phỏng Té Ngã 15S</span>
        </button>

        <button
          onClick={() => setIsRotating(!isRotating)}
          className="px-3 py-1.5 glass-panel rounded-xl text-xs font-semibold text-cyan-300 hover:text-white flex items-center space-x-1.5 transition-all"
        >
          <RefreshCw className={`w-3.5 h-3.5 ${isRotating ? 'animate-spin' : ''}`} />
          <span>{isRotating ? 'Đang Xoay 360°' : 'Xoay Chuột'}</span>
        </button>
      </div>

      {/* 3D Canvas Container */}
      <div ref={mountRef} className="w-full h-full cursor-grab active:cursor-grabbing" />

      {/* 3 Clean Tabs Underneath Watch */}
      <div className="absolute bottom-3 left-3 right-3 sm:left-6 sm:right-6 bg-slate-950/95 border border-slate-800 p-1.5 rounded-2xl shadow-2xl z-10 grid grid-cols-3 gap-1">
        <button
          onClick={() => setActiveScreenTab('HOME')}
          className={`py-2 px-2 rounded-xl text-[11px] font-bold transition-all text-center truncate ${
            activeScreenTab === 'HOME'
              ? 'bg-cyan-500 text-slate-950 shadow-md shadow-cyan-500/20'
              : 'text-slate-400 hover:text-white'
          }`}
        >
          Trạng thái nhịp
        </button>

        <button
          onClick={() => setActiveScreenTab('HR')}
          className={`py-2 px-2 rounded-xl text-[11px] font-bold transition-all text-center truncate ${
            activeScreenTab === 'HR'
              ? 'bg-cyan-500 text-slate-950 shadow-md shadow-cyan-500/20'
              : 'text-slate-400 hover:text-white'
          }`}
        >
          Sóng nhịp tim
        </button>

        <button
          onClick={() => setActiveScreenTab('FALL')}
          className={`py-2 px-2 rounded-xl text-[11px] font-bold transition-all text-center truncate ${
            activeScreenTab === 'FALL'
              ? 'bg-cyan-500 text-slate-950 shadow-md shadow-cyan-500/20'
              : 'text-slate-400 hover:text-white'
          }`}
        >
          Dự đoán Té ngã AI
        </button>
      </div>

    </div>
  );
}
