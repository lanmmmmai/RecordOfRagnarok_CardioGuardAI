import React, { useEffect, useRef } from 'react';
import * as THREE from 'three';

export default function HeartHologram({ bpm = 76 }) {
  const mountRef = useRef(null);

  useEffect(() => {
    const currentMount = mountRef.current;
    if (!currentMount) return;

    const width = currentMount.clientWidth;
    const height = currentMount.clientHeight;

    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 100);
    camera.position.set(0, 0, 4.5);

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(width, height);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    currentMount.appendChild(renderer.domElement);

    // Heart 3D Geometry Shape Function
    const heartShape = new THREE.Shape();
    const x = 0, y = 0;
    heartShape.moveTo(x + 0.25, y + 0.25);
    heartShape.bezierCurveTo(x + 0.25, y + 0.25, x + 0.2, y, x, y);
    heartShape.bezierCurveTo(x - 0.3, y, x - 0.3, y + 0.35, x - 0.3, y + 0.35);
    heartShape.bezierCurveTo(x - 0.3, y + 0.55, x - 0.1, y + 0.77, x + 0.25, y + 0.95);
    heartShape.bezierCurveTo(x + 0.6, y + 0.77, x + 0.8, y + 0.55, x + 0.8, y + 0.35);
    heartShape.bezierCurveTo(x + 0.8, y + 0.35, x + 0.8, y, x + 0.5, y);
    heartShape.bezierCurveTo(x + 0.35, y, x + 0.25, y + 0.25, x + 0.25, y + 0.25);

    const extrudeSettings = {
      depth: 0.3,
      bevelEnabled: true,
      bevelSegments: 5,
      steps: 2,
      bevelSize: 0.1,
      bevelThickness: 0.1
    };

    const heartGeo = new THREE.ExtrudeGeometry(heartShape, extrudeSettings);
    heartGeo.center();

    // Hologram Wireframe Material
    const wireframeMat = new THREE.MeshBasicMaterial({
      color: 0xff3366,
      wireframe: true,
      transparent: true,
      opacity: 0.85
    });

    const heartMesh = new THREE.Mesh(heartGeo, wireframeMat);
    heartMesh.rotation.x = Math.PI; // Flip upside down to align heart orientation
    scene.add(heartMesh);

    // Outer Aura Ring
    const ringGeo = new THREE.TorusGeometry(1.4, 0.02, 16, 64);
    const ringMat = new THREE.MeshBasicMaterial({
      color: 0x00f2fe,
      transparent: true,
      opacity: 0.6
    });
    const ring = new THREE.Mesh(ringGeo, ringMat);
    scene.add(ring);

    // Pulsing Particles
    const particlesCount = 80;
    const posArr = new Float32Array(particlesCount * 3);
    for (let i = 0; i < particlesCount * 3; i += 3) {
      posArr[i] = (Math.random() - 0.5) * 3;
      posArr[i + 1] = (Math.random() - 0.5) * 3;
      posArr[i + 2] = (Math.random() - 0.5) * 3;
    }
    const partGeo = new THREE.BufferGeometry();
    partGeo.setAttribute('position', new THREE.BufferAttribute(posArr, 3));
    const partMat = new THREE.PointsMaterial({
      size: 0.04,
      color: 0xff3366,
      transparent: true,
      opacity: 0.8
    });
    const particles = new THREE.Points(partGeo, partMat);
    scene.add(particles);

    // Animation variables
    let animationFrameId;
    let clock = new THREE.Clock();

    const animate = () => {
      animationFrameId = requestAnimationFrame(animate);
      const elapsedTime = clock.getElapsedTime();

      // Beat calculation based on BPM
      const beatFrequency = (bpm / 60) * Math.PI * 2;
      const pulseScale = 1 + Math.sin(elapsedTime * beatFrequency) * 0.12;

      heartMesh.scale.set(pulseScale, pulseScale, pulseScale);
      heartMesh.rotation.y = elapsedTime * 0.5;

      ring.rotation.x = elapsedTime * 0.3;
      ring.rotation.y = elapsedTime * 0.4;

      particles.rotation.y = -elapsedTime * 0.2;

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
      cancelAnimationFrame(animationFrameId);
      if (currentMount.contains(renderer.domElement)) {
        currentMount.removeChild(renderer.domElement);
      }
      renderer.dispose();
    };
  }, [bpm]);

  return (
    <div className="w-full h-full relative flex flex-col items-center justify-center overflow-hidden">
      <div ref={mountRef} className="w-full h-full min-h-[110px]" />
      <div className="absolute bottom-1.5 flex items-center space-x-2 bg-slate-900/90 px-2.5 py-0.5 rounded-full border border-rose-500/40 text-[11px] font-semibold text-rose-400 shadow-md">
        <span className="w-2 h-2 rounded-full bg-rose-500 animate-ping" />
        <span className="truncate">Hologram Nhịp Tim: {bpm} BPM</span>
      </div>
    </div>
  );
}
